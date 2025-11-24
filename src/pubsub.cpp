#include "bosbase/services/pubsub.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "bosbase/client.h"

namespace bosbase {

PubSubService::PubSubService(BosBase& client) : BaseService(client) {
    client_.init_asio();
    client_.set_tls_init_handler([](websocketpp::connection_hdl) {
        namespace asio = websocketpp::lib::asio;
        auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);
        ctx->set_default_verify_paths();
        ctx->set_verify_mode(asio::ssl::verify_none);
        return ctx;
    });

    client_.set_open_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(mutex_);
        hdl_ = hdl;
        ready_ = true;
    });

    client_.set_message_handler([this](websocketpp::connection_hdl, ws_client::message_ptr msg) {
        try {
            auto payload = nlohmann::json::parse(msg->get_payload());
            handleMessage(payload);
        } catch (...) {
            // ignore malformed messages
        }
    });

    auto on_close = [this](websocketpp::connection_hdl) {
        std::lock_guard<std::mutex> lock(mutex_);
        ready_ = false;
        if (!manual_close_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            ensureSocket();
        }
    };
    client_.set_close_handler(on_close);
    client_.set_fail_handler(on_close);
}

PubSubService::~PubSubService() {
    disconnect();
}

std::string PubSubService::buildWsUrl() const {
    std::map<std::string, nlohmann::json> query;
    if (client.authStore() && client.authStore()->isValid()) {
        query["token"] = client.authStore()->token();
    }
    auto url = client.buildUrl("/api/pubsub", query);
    if (url.rfind("https://", 0) == 0) {
        url.replace(0, 5, "wss");
    } else if (url.rfind("http://", 0) == 0) {
        url.replace(0, 4, "ws");
    } else {
        url = "ws://" + url;
    }
    return url;
}

void PubSubService::ensureSocket() {
    if (ready_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (ready_) return;
    manual_close_ = false;

    websocketpp::lib::error_code ec;
    auto con = client_.get_connection(buildWsUrl(), ec);
    if (ec) {
        throw ClientResponseError({}, 0, {{"message", ec.message()}});
    }
    hdl_ = con->get_handle();
    client_.connect(con);
    if (!thread_.joinable()) {
        thread_ = std::thread([this]() { client_.run(); });
    }

    // wait for connection
    auto start = std::chrono::steady_clock::now();
    while (!ready_ && std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < 10.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (!ready_) {
        throw ClientResponseError({}, 0, {{"message", "PubSub connection not established"}});
    }
}

PubSubMessage PubSubService::publish(const std::string& topic, const nlohmann::json& data) {
    if (topic.empty()) throw std::invalid_argument("topic must be set");
    ensureSocket();

    nlohmann::json payload;
    payload["type"] = "publish";
    payload["topic"] = topic;
    payload["data"] = data;
    sendEnvelope(payload);

    PubSubMessage ack;
    ack.topic = topic;
    ack.data = data;
    ack.created = "";
    return ack;
}

std::function<void()> PubSubService::subscribe(const std::string& topic, std::function<void(const PubSubMessage&)> callback) {
    if (topic.empty()) throw std::invalid_argument("topic must be set");

    bool first_listener = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& listeners = subscriptions_[topic];
        if (listeners.empty()) first_listener = true;
        listeners.push_back(callback);
    }

    ensureSocket();
    if (first_listener) {
        nlohmann::json payload;
        payload["type"] = "subscribe";
        payload["topic"] = topic;
        sendEnvelope(payload);
    }

    return [this, topic, callback]() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscriptions_.find(topic);
        if (it == subscriptions_.end()) return;
        auto& vec = it->second;
        vec.erase(
            std::remove_if(vec.begin(), vec.end(), [&](const auto& fn) {
                return fn.target_type() == callback.target_type();
            }),
            vec.end());
        if (vec.empty()) {
            subscriptions_.erase(it);
            nlohmann::json payload;
            payload["type"] = "unsubscribe";
            payload["topic"] = topic;
            sendEnvelope(payload);
        }
        if (subscriptions_.empty()) {
            disconnect();
        }
    };
}

void PubSubService::unsubscribe(const std::optional<std::string>& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (topic) {
        subscriptions_.erase(*topic);
        nlohmann::json payload;
        payload["type"] = "unsubscribe";
        payload["topic"] = *topic;
        sendEnvelope(payload);
    } else {
        subscriptions_.clear();
        nlohmann::json payload;
        payload["type"] = "unsubscribe";
        sendEnvelope(payload);
    }
    if (subscriptions_.empty()) disconnect();
}

void PubSubService::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    manual_close_ = true;
    if (ready_) {
        websocketpp::lib::error_code ec;
        client_.close(hdl_, websocketpp::close::status::normal, "closing", ec);
    }
    ready_ = false;
    if (thread_.joinable()) {
        client_.stop();
        thread_.join();
    }
}

void PubSubService::sendEnvelope(const nlohmann::json& payload) {
    if (!ready_) return;
    websocketpp::lib::error_code ec;
    client_.send(hdl_, payload.dump(), websocketpp::frame::opcode::text, ec);
}

void PubSubService::handleMessage(const nlohmann::json& payload) {
    auto topic = payload.value("topic", std::string{});
    auto data = payload.value("data", nlohmann::json{});
    PubSubMessage msg;
    msg.id = payload.value("id", std::string{});
    msg.topic = topic;
    msg.created = payload.value("created", std::string{});
    msg.data = data;

    std::vector<std::function<void(const PubSubMessage&)>> listeners;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscriptions_.find(topic);
        if (it != subscriptions_.end()) {
            listeners = it->second;
        }
    }
    for (auto& cb : listeners) {
        try {
            cb(msg);
        } catch (...) {
        }
    }
}

} // namespace bosbase

#include "bosbase/services/realtime.h"

#include <algorithm>
#include <chrono>
#include <curl/curl.h>
#include <sstream>
#include <thread>

#include "bosbase/client.h"

namespace bosbase {

RealtimeService::RealtimeService(BosBase& client) : BaseService(client) {}

RealtimeService::~RealtimeService() {
    disconnect();
}

std::function<void()> RealtimeService::subscribe(
    const std::string& topic,
    std::function<void(const nlohmann::json&)> callback,
    const std::map<std::string, nlohmann::json>&,
    const std::map<std::string, std::string>&) {
    if (topic.empty()) {
        throw std::invalid_argument("topic must be set");
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    subscriptions_[topic].push_back(callback);
    ensureThread();
    ensureConnected();
    submitSubscriptions();
    return [this, topic, callback]() {
        unsubscribeByTopicAndListener(topic, callback);
    };
}

void RealtimeService::unsubscribe(const std::optional<std::string>& topic) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (topic) {
            subscriptions_.erase(*topic);
        } else {
            subscriptions_.clear();
        }
    }
    submitSubscriptions();
    if (subscriptions_.empty()) {
        disconnect();
    }
}

void RealtimeService::unsubscribeByPrefix(const std::string& prefix) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto it = subscriptions_.begin(); it != subscriptions_.end();) {
        if (it->first.rfind(prefix, 0) == 0) {
            it = subscriptions_.erase(it);
        } else {
            ++it;
        }
    }
    submitSubscriptions();
    if (subscriptions_.empty()) {
        disconnect();
    }
}

void RealtimeService::unsubscribeByTopicAndListener(const std::string& topic, const std::function<void(const nlohmann::json&)>& listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = subscriptions_.find(topic);
    if (it == subscriptions_.end()) return;
    auto& vec = it->second;
    vec.erase(
        std::remove_if(vec.begin(), vec.end(), [&](const auto& fn) {
            return fn.target_type() == listener.target_type();
        }),
        vec.end());
    if (vec.empty()) {
        subscriptions_.erase(it);
    }
    submitSubscriptions();
    if (subscriptions_.empty()) {
        disconnect();
    }
}

void RealtimeService::ensureConnected(double timeoutSeconds) {
    auto start = std::chrono::steady_clock::now();
    ensureThread();
    while (!ready_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeoutSeconds) {
            throw ClientResponseError(client.buildUrl("/api/realtime"), 0, {{"message", "Realtime connection not established"}});
        }
    }
}

void RealtimeService::disconnect() {
    stop_ = true;
    ready_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void RealtimeService::ensureThread() {
    if (worker_.joinable()) return;
    stop_ = false;
    worker_ = std::thread([this]() { run(); });
}

size_t RealtimeService::realtimeWrite(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto self = static_cast<RealtimeService*>(userdata);
    size_t total = size * nmemb;
    static std::string buffer;
    buffer.append(ptr, total);

    size_t pos = 0;
    while (true) {
        auto newline = buffer.find('\n', pos);
        if (newline == std::string::npos) {
            buffer.erase(0, pos);
            break;
        }
        std::string line = buffer.substr(pos, newline - pos);
        pos = newline + 1;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        static std::string eventName = "message";
        static std::string data;
        static std::string id;
        if (line.empty()) {
            nlohmann::json payload;
            if (!data.empty()) {
                try {
                    payload = nlohmann::json::parse(data);
                } catch (...) {
                    payload = nlohmann::json{{"raw", data}};
                }
            }
            self->handleEvent(eventName, payload);
            eventName = "message";
            data.clear();
            id.clear();
            continue;
        }

        if (line[0] == ':') {
            continue;
        }
        auto colon = line.find(':');
        std::string field = colon == std::string::npos ? line : line.substr(0, colon);
        std::string value = colon == std::string::npos ? "" : line.substr(colon + 1);
        if (!value.empty() && value.front() == ' ') value.erase(0, 1);

        if (field == "event") {
            eventName = value.empty() ? "message" : value;
        } else if (field == "data") {
            data += value + "\n";
        } else if (field == "id") {
            id = value;
        }
    }
    return total;
}

int RealtimeService::realtimeProgress(void* clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    auto self = static_cast<RealtimeService*>(clientp);
    return self->stop_.load() ? 1 : 0;
}

void RealtimeService::run() {
    while (!stop_) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        std::string url = client.buildUrl("/api/realtime");
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: text/event-stream");
        headers = curl_slist_append(headers, "Cache-Control: no-store");
        std::string acceptLang = "Accept-Language: " + client.language();
        headers = curl_slist_append(headers, acceptLang.c_str());
        std::string ua = "User-Agent: bosbase-cpp-sdk";
        headers = curl_slist_append(headers, ua.c_str());
        std::string auth;
        if (client.authStore() && client.authStore()->isValid()) {
            auth = "Authorization: " + client.authStore()->token();
            headers = curl_slist_append(headers, auth.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, realtimeWrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "bosbase-cpp-sdk");
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, realtimeProgress);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);

        auto res = curl_easy_perform(curl);
        (void)res;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        ready_ = false;
        client_id_.clear();

        if (stop_) break;

        if (onDisconnect) {
            std::vector<std::string> active;
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                for (const auto& kv : subscriptions_) active.push_back(kv.first);
            }
            onDisconnect(active);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void RealtimeService::handleEvent(const std::string& event, const nlohmann::json& payload) {
    if (event == "PB_CONNECT") {
        client_id_ = payload.value("clientId", std::string{});
        ready_ = true;
        submitSubscriptions();
        if (onDisconnect) {
            try {
                onDisconnect({});
            } catch (...) {}
        }
        return;
    }

    std::vector<std::function<void(const nlohmann::json&)>> listeners;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto it = subscriptions_.find(event);
        if (it != subscriptions_.end()) {
            listeners = it->second;
        }
    }
    for (auto& cb : listeners) {
        try {
            cb(payload);
        } catch (...) {
            // ignore listener errors
        }
    }
}

void RealtimeService::submitSubscriptions() {
    if (!ready_ || client_id_.empty()) return;
    std::vector<std::string> active;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (const auto& kv : subscriptions_) {
            if (!kv.second.empty()) active.push_back(kv.first);
        }
    }
    if (active.empty()) return;
    nlohmann::json payload;
    payload["clientId"] = client_id_;
    payload["subscriptions"] = active;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    try {
        client.send("/api/realtime", std::move(opts));
    } catch (const ClientResponseError& err) {
        if (err.isAbort()) {
            return;
        }
        throw;
    }
}

} // namespace bosbase

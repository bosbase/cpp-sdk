#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include "bosbase/services/base.h"

namespace bosbase {

struct PubSubMessage {
    std::string id;
    std::string topic;
    std::string created;
    nlohmann::json data;
};

class PubSubService : public BaseService {
public:
    explicit PubSubService(BosBase& client);
    ~PubSubService();

    PubSubMessage publish(const std::string& topic, const nlohmann::json& data);
    std::function<void()> subscribe(const std::string& topic, std::function<void(const PubSubMessage&)> callback);
    void unsubscribe(const std::optional<std::string>& topic = std::nullopt);
    void disconnect();

    bool isConnected() const { return ready_; }

private:
    using ws_client = websocketpp::client<websocketpp::config::asio_tls_client>;

    ws_client client_;
    websocketpp::connection_hdl hdl_;
    std::thread thread_;
    std::mutex mutex_;
    bool ready_ = false;
    bool manual_close_ = false;
    std::map<std::string, std::vector<std::function<void(const PubSubMessage&)>>> subscriptions_;

    void ensureSocket();
    void sendEnvelope(const nlohmann::json& payload);
    std::string buildWsUrl() const;
    void handleMessage(const nlohmann::json& payload);
};

} // namespace bosbase

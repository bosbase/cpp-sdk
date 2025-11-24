#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>

#include "bosbase/services/base.h"

namespace bosbase {

class RealtimeService : public BaseService {
public:
    explicit RealtimeService(BosBase& client);
    ~RealtimeService();

    std::function<void()> subscribe(
        const std::string& topic,
        std::function<void(const nlohmann::json&)> callback,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    void unsubscribe(const std::optional<std::string>& topic = std::nullopt);
    void unsubscribeByPrefix(const std::string& prefix);
    void unsubscribeByTopicAndListener(const std::string& topic, const std::function<void(const nlohmann::json&)>& listener);

    void ensureConnected(double timeoutSeconds = 10.0);
    void disconnect();

    std::function<void(const std::vector<std::string>&)> onDisconnect;
    bool isConnected() const { return ready_; }
    std::string clientId() const { return client_id_; }

private:
    std::string client_id_;
    std::map<std::string, std::vector<std::function<void(const nlohmann::json&)>>> subscriptions_;
    std::thread worker_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> ready_{false};
    std::recursive_mutex mutex_;

    static size_t realtimeWrite(char* ptr, size_t size, size_t nmemb, void* userdata);
    static int realtimeProgress(void* clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t);
    void ensureThread();
    void run();
    void handleEvent(const std::string& event, const nlohmann::json& payload);
    void submitSubscriptions();
};

} // namespace bosbase

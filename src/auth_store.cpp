#include "bosbase/auth_store.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "bosbase/utils.h"

namespace bosbase {

bool AuthStore::isValid() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (token_.empty()) {
        return false;
    }
    return isJwtValid(token_);
}

void AuthStore::save(const std::string& token, const nlohmann::json& record) {
    std::vector<Listener> callbacks;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        token_ = token;
        record_ = record;
        callbacks = listeners_;
    }
    for (auto& cb : callbacks) {
        try {
            cb(token_, record_);
        } catch (...) {
            // best effort notification
        }
    }
}

void AuthStore::clear() {
    save({}, {});
}

void AuthStore::addListener(Listener listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    listeners_.push_back(std::move(listener));
}

void AuthStore::removeListener(Listener listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    listeners_.erase(
        std::remove_if(
            listeners_.begin(),
            listeners_.end(),
            [&](const Listener& l) {
                // cannot compare std::function reliably; best effort by target_type
                return l.target_type() == listener.target_type();
            }),
        listeners_.end());
}

bool AuthStore::isJwtValid(const std::string& token) {
    auto parts_end = std::count(token.begin(), token.end(), '.');
    if (parts_end != 2) {
        return false;
    }
    auto first_dot = token.find('.');
    auto second_dot = token.find('.', first_dot + 1);
    if (first_dot == std::string::npos || second_dot == std::string::npos) {
        return false;
    }
    auto payload_encoded = token.substr(first_dot + 1, second_dot - first_dot - 1);
    auto decoded = base64UrlDecode(payload_encoded);
    if (decoded.empty()) {
        return false;
    }

    try {
        auto payload = nlohmann::json::parse(decoded);
        if (!payload.contains("exp")) {
            return false;
        }
        auto exp = payload["exp"];
        if (!exp.is_number()) {
            return false;
        }
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return exp.get<long long>() > now;
    } catch (...) {
        return false;
    }
}

} // namespace bosbase

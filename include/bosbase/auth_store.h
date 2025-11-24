#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>

namespace bosbase {

class AuthStore {
public:
    using Listener = std::function<void(const std::string&, const nlohmann::json&)>;

    AuthStore() = default;

    const std::string& token() const { return token_; }
    const nlohmann::json& record() const { return record_; }

    bool isValid() const;

    void save(const std::string& token, const nlohmann::json& record);
    void clear();

    void addListener(Listener listener);
    void removeListener(Listener listener);

private:
    static bool isJwtValid(const std::string& token);

    std::string token_;
    nlohmann::json record_{};
    mutable std::recursive_mutex mutex_;
    std::vector<Listener> listeners_;
};

} // namespace bosbase

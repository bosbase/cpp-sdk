#pragma once

#include <stdexcept>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace bosbase {

class ClientResponseError : public std::runtime_error {
public:
    ClientResponseError(
        const std::string& url = {},
        long status = 0,
        const nlohmann::json& response = {},
        bool is_abort = false,
        const std::string& original_error = {})
        : std::runtime_error(buildMessage(url, status, response, is_abort, original_error)),
          url_(url),
          status_(status),
          response_(response),
          is_abort_(is_abort),
          original_error_(original_error) {}

    const std::string& url() const { return url_; }
    long status() const { return status_; }
    const nlohmann::json& response() const { return response_; }
    bool isAbort() const { return is_abort_; }
    const std::string& originalError() const { return original_error_; }

private:
    static std::string buildMessage(const std::string& url,
                                    long status,
                                    const nlohmann::json& response,
                                    bool is_abort,
                                    const std::string& original_error) {
        std::string resp = response.is_null() ? "{}" : response.dump();
        return "ClientResponseError(status=" + std::to_string(status) +
               ", url=" + url + ", response=" + resp +
               ", is_abort=" + (is_abort ? "true" : "false") +
               (original_error.empty() ? "" : ", original_error=" + original_error) + ")";
    }

    std::string url_;
    long status_;
    nlohmann::json response_;
    bool is_abort_;
    std::string original_error_;
};

} // namespace bosbase

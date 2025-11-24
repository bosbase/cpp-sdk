#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace bosbase {

struct FileAttachment {
    std::string field;
    std::string filename;
    std::string contentType{"application/octet-stream"};
    std::vector<unsigned char> data;
};

struct SendOptions {
    std::string method = "GET";
    std::map<std::string, std::string> headers;
    std::map<std::string, nlohmann::json> query;
    nlohmann::json body;
    std::vector<FileAttachment> files;
    std::optional<long> timeoutMs;
};

using BeforeSendHook = std::function<void(std::string&, SendOptions&)>;
using AfterSendHook = std::function<nlohmann::json(long, const std::map<std::string, std::string>&, const nlohmann::json&)>;

} // namespace bosbase

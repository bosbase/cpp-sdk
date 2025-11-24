#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace bosbase {

inline std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase << std::setw(2) << int(c);
        }
    }
    return escaped.str();
}

inline std::string encodePathSegment(const std::string& segment) {
    return urlEncode(segment);
}

inline std::string buildQuery(const std::map<std::string, std::vector<std::string>>& params) {
    std::ostringstream ss;
    bool first = true;
    for (const auto& [key, values] : params) {
        for (const auto& value : values) {
            if (!first) {
                ss << "&";
            }
            first = false;
            ss << urlEncode(key) << "=" << urlEncode(value);
        }
    }
    return ss.str();
}

inline std::map<std::string, std::vector<std::string>> normalizeQuery(const std::map<std::string, nlohmann::json>& params) {
    std::map<std::string, std::vector<std::string>> normalized;
    for (const auto& [key, value] : params) {
        if (value.is_null()) {
            continue;
        }
        if (value.is_array()) {
            std::vector<std::string> list;
            for (const auto& v : value) {
                if (v.is_null()) continue;
                list.push_back(v.dump());
            }
            if (!list.empty()) {
                normalized[key] = list;
            }
        } else {
            normalized[key] = { value.is_string() ? value.get<std::string>() : value.dump() };
        }
    }
    return normalized;
}

inline nlohmann::json toSerializable(const nlohmann::json& value) {
    return value;
}

inline std::string buildRelativeUrl(const std::string& path, const std::map<std::string, nlohmann::json>& query) {
    std::string rel = "/" + std::string(path.begin() + (path.size() > 0 && path.front() == '/' ? 1 : 0), path.end());
    if (!query.empty()) {
        auto norm = normalizeQuery(query);
        auto qs = buildQuery(norm);
        if (!qs.empty()) {
            rel += "?" + qs;
        }
    }
    return rel;
}

inline std::string base64UrlDecode(const std::string& input) {
    const std::string BASE64_CHARS =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    auto isBase64 = [](unsigned char c) {
        return (isalnum(c) || c == '+' || c == '/');
    };

    std::string data = input;
    std::replace(data.begin(), data.end(), '-', '+');
    std::replace(data.begin(), data.end(), '_', '/');
    while (data.size() % 4 != 0) {
        data.push_back('=');
    }

    int in_len = static_cast<int>(data.size());
    int i = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && data[in_] != '=' && isBase64(data[in_])) {
        char_array_4[i++] = data[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<unsigned char>(BASE64_CHARS.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 4; j++)
            char_array_4[j] = 0;
        for (int j = 0; j < 4; j++)
            char_array_4[j] = static_cast<unsigned char>(BASE64_CHARS.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (int j = 0; j < i - 1; j++) ret += char_array_3[j];
    }

    return ret;
}

} // namespace bosbase

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <stdexcept>

#include "bosbase/services/base.h"
#include "bosbase/types.h"

namespace bosbase {

class SQLService : public BaseService {
public:
    explicit SQLService(BosBase& client) : BaseService(client) {}

    SQLExecuteResponse execute(
        const std::string& query,
        const std::map<std::string, nlohmann::json>& query_params = {},
        const std::map<std::string, std::string>& headers = {}) {
        auto trimmed = trim(query);
        if (trimmed.empty()) {
            throw std::invalid_argument("query is required");
        }

        SQLExecuteRequest payload{trimmed};

        SendOptions opts;
        opts.method = "POST";
        opts.body = payload.toJson();
        opts.query = query_params;
        opts.headers = headers;
        auto data = client.send("/api/sql/execute", std::move(opts));
        return SQLExecuteResponse::fromJson(data);
    }

private:
    static std::string trim(const std::string& input) {
        auto start = std::find_if_not(input.begin(), input.end(), [](unsigned char c) { return std::isspace(c); });
        auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) { return std::isspace(c); }).base();
        if (start >= end) {
            return {};
        }
        return std::string(start, end);
    }
};

} // namespace bosbase

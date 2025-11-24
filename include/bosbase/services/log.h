#pragma once

#include <map>
#include <optional>
#include <string>

#include "bosbase/services/base.h"

namespace bosbase {

class LogService : public BaseService {
public:
    explicit LogService(BosBase& client) : BaseService(client) {}

    nlohmann::json getList(
        int page = 1,
        int per_page = 30,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        auto params = query;
        params.emplace("page", page);
        params.emplace("perPage", per_page);
        SendOptions opts;
        opts.query = params;
        opts.headers = headers;
        return client.send("/api/logs", std::move(opts));
    }

    nlohmann::json getOne(
        const std::string& id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/logs/" + encodePathSegment(id), std::move(opts));
    }

    nlohmann::json getStats(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/logs/stats", std::move(opts));
    }
};

} // namespace bosbase

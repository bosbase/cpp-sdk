#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"

namespace bosbase {

class CronService : public BaseService {
public:
    explicit CronService(BosBase& client) : BaseService(client) {}

    nlohmann::json getFullList(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/crons", std::move(opts));
    }

    nlohmann::json run(
        const std::string& job_id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/crons/" + encodePathSegment(job_id), std::move(opts));
    }
};

} // namespace bosbase

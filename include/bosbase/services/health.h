#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"

namespace bosbase {

class HealthService : public BaseService {
public:
    explicit HealthService(BosBase& client) : BaseService(client) {}

    nlohmann::json check(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/health", std::move(opts));
    }
};

} // namespace bosbase

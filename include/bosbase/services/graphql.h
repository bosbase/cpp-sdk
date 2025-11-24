#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"

namespace bosbase {

class GraphQLService : public BaseService {
public:
    explicit GraphQLService(BosBase& client) : BaseService(client) {}

    nlohmann::json sendQuery(
        const std::string& query_string,
        const nlohmann::json& variables = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["query"] = query_string;
        if (!variables.is_null() && !variables.empty()) {
            payload["variables"] = variables;
        }
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/graphql", std::move(opts));
    }
};

} // namespace bosbase

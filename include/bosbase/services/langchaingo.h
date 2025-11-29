#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"
#include "bosbase/types.h"

namespace bosbase {

class LangChaingoService : public BaseService {
public:
    explicit LangChaingoService(BosBase& client) : BaseService(client) {}

    nlohmann::json completions(
        const LangChaingoCompletionRequest& payload,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/langchaingo/completions", std::move(opts));
    }

    nlohmann::json rag(
        const LangChaingoRAGRequest& payload,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/langchaingo/rag", std::move(opts));
    }

    nlohmann::json queryDocuments(
        const LangChaingoDocumentQueryRequest& payload,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/langchaingo/documents/query", std::move(opts));
    }

    nlohmann::json sql(
        const LangChaingoSQLRequest& payload,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/langchaingo/sql", std::move(opts));
    }
};

} // namespace bosbase

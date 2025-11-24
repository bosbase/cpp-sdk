#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"
#include "bosbase/types.h"

namespace bosbase {

class LLMDocumentService : public BaseService {
public:
    explicit LLMDocumentService(BosBase& client) : BaseService(client) {}

    nlohmann::json listCollections(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        auto data = client.send("/api/llm-documents/collections", std::move(opts));
        return data.is_array() ? data : nlohmann::json::array({data});
    }

    void createCollection(
        const std::string& name,
        const std::optional<std::map<std::string, std::string>>& metadata = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        if (metadata) payload["metadata"] = *metadata;
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        client.send("/api/llm-documents/collections/" + encodePathSegment(name), std::move(opts));
    }

    void deleteCollection(
        const std::string& name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send("/api/llm-documents/collections/" + encodePathSegment(name), std::move(opts));
    }

    nlohmann::json insert(
        const std::string& collection,
        const LLMDocument& document,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = document.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/llm-documents/" + encodePathSegment(collection), std::move(opts));
    }

    LLMDocument get(
        const std::string& collection,
        const std::string& document_id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        auto data = client.send("/api/llm-documents/" + encodePathSegment(collection) + "/" + encodePathSegment(document_id), std::move(opts));
        return LLMDocument::fromJson(data);
    }

    nlohmann::json update(
        const std::string& collection,
        const std::string& document_id,
        const LLMDocumentUpdate& document,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = document.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/llm-documents/" + encodePathSegment(collection) + "/" + encodePathSegment(document_id), std::move(opts));
    }

    void remove(
        const std::string& collection,
        const std::string& document_id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send("/api/llm-documents/" + encodePathSegment(collection) + "/" + encodePathSegment(document_id), std::move(opts));
    }

    nlohmann::json list(
        const std::string& collection,
        std::optional<int> page = std::nullopt,
        std::optional<int> per_page = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        auto params = query;
        if (page) params["page"] = *page;
        if (per_page) params["perPage"] = *per_page;
        SendOptions opts;
        opts.query = params;
        opts.headers = headers;
        return client.send("/api/llm-documents/" + encodePathSegment(collection), std::move(opts));
    }

    nlohmann::json query(
        const std::string& collection,
        const LLMQueryOptions& options,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = options.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/llm-documents/" + encodePathSegment(collection) + "/documents/query", std::move(opts));
    }
};

} // namespace bosbase

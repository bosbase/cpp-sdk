#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"
#include "bosbase/types.h"

namespace bosbase {

class VectorService : public BaseService {
public:
    explicit VectorService(BosBase& client) : BaseService(client) {}

    nlohmann::json createCollection(
        const std::string& name,
        const std::optional<VectorCollectionConfig>& config = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["name"] = name;
        if (config) payload["config"] = config->toJson();
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/collections", std::move(opts));
    }

    nlohmann::json listCollections(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/collections", std::move(opts));
    }

    void deleteCollection(
        const std::string& name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send("/api/vector/collections/" + encodePathSegment(name), std::move(opts));
    }

    nlohmann::json insert(
        const VectorDocument& document,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = document.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/documents", std::move(opts));
    }

    nlohmann::json batchInsert(
        const VectorBatchInsertOptions& options,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = options.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/documents/batch", std::move(opts));
    }

    nlohmann::json get(
        const std::string& id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/documents/" + encodePathSegment(id), std::move(opts));
    }

    nlohmann::json update(
        const std::string& id,
        const VectorDocument& document,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = document.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/documents/" + encodePathSegment(id), std::move(opts));
    }

    void remove(
        const std::string& id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send("/api/vector/documents/" + encodePathSegment(id), std::move(opts));
    }

    nlohmann::json list(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/documents", std::move(opts));
    }

    nlohmann::json search(
        const VectorSearchOptions& options,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = options.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/vector/search", std::move(opts));
    }
};

} // namespace bosbase

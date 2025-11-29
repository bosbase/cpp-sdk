#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"
#include "bosbase/types.h"

namespace bosbase {

class VectorService : public BaseService {
public:
    explicit VectorService(BosBase& client) : BaseService(client) {}

private:
    static std::string basePath() {
        return "/api/vectors";
    }

    static std::string collectionPath(const std::string& collection) {
        return basePath() + "/" + encodePathSegment(collection);
    }

public:
    nlohmann::json createCollection(
        const std::string& name,
        const std::optional<VectorCollectionConfig>& config = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload = config ? config->toJson() : nlohmann::json::object();
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send(basePath() + "/collections/" + encodePathSegment(name), std::move(opts));
    }

    nlohmann::json updateCollection(
        const std::string& name,
        const std::optional<VectorCollectionConfig>& config = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload = config ? config->toJson() : nlohmann::json::object();
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send(basePath() + "/collections/" + encodePathSegment(name), std::move(opts));
    }

    nlohmann::json listCollections(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send(basePath() + "/collections", std::move(opts));
    }

    void deleteCollection(
        const std::string& name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send(basePath() + "/collections/" + encodePathSegment(name), std::move(opts));
    }

    nlohmann::json insert(
        const std::string& collection,
        const VectorDocument& document,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = document.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send(collectionPath(collection), std::move(opts));
    }

    nlohmann::json batchInsert(
        const std::string& collection,
        const VectorBatchInsertOptions& options,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = options.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send(collectionPath(collection) + "/documents/batch", std::move(opts));
    }

    nlohmann::json get(
        const std::string& collection,
        const std::string& id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send(collectionPath(collection) + "/" + encodePathSegment(id), std::move(opts));
    }

    nlohmann::json update(
        const std::string& collection,
        const std::string& id,
        const nlohmann::json& document,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = document.is_null() ? nlohmann::json::object() : document;
        opts.query = query;
        opts.headers = headers;
        return client.send(collectionPath(collection) + "/" + encodePathSegment(id), std::move(opts));
    }

    void remove(
        const std::string& collection,
        const std::string& id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send(collectionPath(collection) + "/" + encodePathSegment(id), std::move(opts));
    }

    nlohmann::json list(
        const std::string& collection,
        const std::optional<int>& page = std::nullopt,
        const std::optional<int>& per_page = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        auto params = query;
        if (page) params["page"] = *page;
        if (per_page) params["perPage"] = *per_page;
        SendOptions opts;
        opts.query = params;
        opts.headers = headers;
        return client.send(collectionPath(collection), std::move(opts));
    }

    nlohmann::json search(
        const std::string& collection,
        const VectorSearchOptions& options,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = options.toJson();
        opts.query = query;
        opts.headers = headers;
        return client.send(collectionPath(collection) + "/documents/search", std::move(opts));
    }
};

} // namespace bosbase

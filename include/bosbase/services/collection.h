#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "bosbase/services/base.h"

namespace bosbase {

class CollectionService : public BaseCrudService {
public:
    explicit CollectionService(BosBase& client) : BaseCrudService(client) {}

    std::string baseCrudPath() const override { return "/api/collections"; }

    void deleteCollection(const std::string& id_or_name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        remove(id_or_name, {}, query, headers);
    }

    void truncate(
        const std::string& id_or_name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send(baseCrudPath() + "/" + encodePathSegment(id_or_name) + "/truncate", std::move(opts));
    }

    nlohmann::json importCollections(
        const nlohmann::json& collections,
        bool delete_missing = false,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        auto params = query;
        params.emplace("deleteMissing", delete_missing);
        SendOptions opts;
        opts.method = "PUT";
        opts.body = collections;
        opts.query = params;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/import", std::move(opts));
    }

    nlohmann::json getScaffolds(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/scaffolds", std::move(opts));
    }

    nlohmann::json createFromScaffold(
        const std::string& type,
        const std::string& name,
        const std::optional<nlohmann::json>& overrides = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["name"] = name;
        if (overrides) payload["overrides"] = *overrides;
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/scaffolds/" + encodePathSegment(type), std::move(opts));
    }

    nlohmann::json createBase(
        const std::string& name,
        const std::optional<nlohmann::json>& overrides = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        return createFromScaffold("base", name, overrides, query, headers);
    }

    nlohmann::json createAuth(
        const std::string& name,
        const std::optional<nlohmann::json>& overrides = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        return createFromScaffold("auth", name, overrides, query, headers);
    }

    nlohmann::json createView(
        const std::string& name,
        const std::optional<std::string>& view_query = std::nullopt,
        const std::optional<nlohmann::json>& overrides = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["name"] = name;
        if (view_query) payload["viewQuery"] = *view_query;
        if (overrides) payload["overrides"] = *overrides;
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/views", std::move(opts));
    }

    nlohmann::json addField(
        const std::string& collection,
        const nlohmann::json& field,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.body = field;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/fields", std::move(opts));
    }

    nlohmann::json updateField(
        const std::string& collection,
        const std::string& field_name,
        const nlohmann::json& updates,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = updates;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/fields/" + encodePathSegment(field_name), std::move(opts));
    }

    void removeField(
        const std::string& collection,
        const std::string& field_name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/fields/" + encodePathSegment(field_name), std::move(opts));
    }

    nlohmann::json getField(
        const std::string& collection,
        const std::string& field_name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/fields/" + encodePathSegment(field_name), std::move(opts));
    }

    nlohmann::json addIndex(
        const std::string& collection,
        const std::vector<std::string>& columns,
        std::optional<bool> unique = std::nullopt,
        const std::optional<std::string>& index_name = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["columns"] = columns;
        if (unique) payload["unique"] = *unique;
        if (index_name) payload["indexName"] = *index_name;
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/indexes", std::move(opts));
    }

    void removeIndex(
        const std::string& collection,
        const std::vector<std::string>& columns,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["columns"] = columns;
        SendOptions opts;
        opts.method = "DELETE";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/indexes", std::move(opts));
    }

    nlohmann::json getIndexes(
        const std::string& collection,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/indexes", std::move(opts));
    }

    nlohmann::json setRules(
        const std::string& collection,
        const nlohmann::json& rules,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = rules;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/rules", std::move(opts));
    }

    nlohmann::json setRule(
        const std::string& collection,
        const std::string& type,
        const std::optional<std::string>& rule,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["rule"] = rule ? nlohmann::json(*rule) : nlohmann::json();
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/rules/" + encodePathSegment(type), std::move(opts));
    }

    nlohmann::json getRules(
        const std::string& collection,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send(baseCrudPath() + "/" + encodePathSegment(collection) + "/rules", std::move(opts));
    }
};

} // namespace bosbase

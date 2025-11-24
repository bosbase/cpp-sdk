#pragma once

#include <map>
#include <optional>
#include <string>

#include "bosbase/services/base.h"

namespace bosbase {

class CacheService : public BaseService {
public:
    explicit CacheService(BosBase& client) : BaseService(client) {}

    nlohmann::json list(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        auto data = client.send("/api/cache", std::move(opts));
        if (data.is_object() && data.contains("items")) {
            return data["items"];
        }
        return data;
    }

    nlohmann::json create(
        const std::string& name,
        std::optional<int> size_bytes = std::nullopt,
        std::optional<int> default_ttl_seconds = std::nullopt,
        std::optional<int> read_timeout_ms = std::nullopt,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload = body;
        payload["name"] = name;
        if (size_bytes) payload["sizeBytes"] = *size_bytes;
        if (default_ttl_seconds) payload["defaultTTLSeconds"] = *default_ttl_seconds;
        if (read_timeout_ms) payload["readTimeoutMs"] = *read_timeout_ms;
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/cache", std::move(opts));
    }

    nlohmann::json update(
        const std::string& name,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/cache/" + encodePathSegment(name), std::move(opts));
    }

    void remove(
        const std::string& name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send("/api/cache/" + encodePathSegment(name), std::move(opts));
    }

    nlohmann::json setEntry(
        const std::string& cache,
        const std::string& key,
        const nlohmann::json& value,
        std::optional<int> ttl_seconds = std::nullopt,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload = body;
        payload["value"] = value;
        if (ttl_seconds) payload["ttlSeconds"] = *ttl_seconds;
        SendOptions opts;
        opts.method = "PUT";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/cache/" + encodePathSegment(cache) + "/entries/" + encodePathSegment(key), std::move(opts));
    }

    nlohmann::json getEntry(
        const std::string& cache,
        const std::string& key,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/cache/" + encodePathSegment(cache) + "/entries/" + encodePathSegment(key), std::move(opts));
    }

    nlohmann::json renewEntry(
        const std::string& cache,
        const std::string& key,
        std::optional<int> ttl_seconds = std::nullopt,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload = body;
        if (ttl_seconds) payload["ttlSeconds"] = *ttl_seconds;
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/cache/" + cache + "/entries/" + key, std::move(opts));
    }

    void deleteEntry(
        const std::string& cache,
        const std::string& key,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send("/api/cache/" + encodePathSegment(cache) + "/entries/" + encodePathSegment(key), std::move(opts));
    }
};

} // namespace bosbase

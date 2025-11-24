#pragma once

#include <map>
#include <string>

#include "bosbase/services/base.h"

namespace bosbase {

class SettingsService : public BaseService {
public:
    explicit SettingsService(BosBase& client) : BaseService(client) {}

    nlohmann::json getAll(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/settings", std::move(opts));
    }

    nlohmann::json update(
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "PATCH";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/settings", std::move(opts));
    }

    nlohmann::json testS3(
        const std::string& filesystem = "storage",
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        auto params = query;
        params.emplace("filesystem", filesystem);
        SendOptions opts;
        opts.query = params;
        opts.headers = headers;
        return client.send("/api/settings/test/s3", std::move(opts));
    }

    nlohmann::json testEmail(
        const std::string& collection,
        const std::string& to_email,
        const std::string& template_name,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["collectionIdOrName"] = collection;
        payload["toEmail"] = to_email;
        payload["template"] = template_name;
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/settings/test/email", std::move(opts));
    }

    nlohmann::json generateAppleClientSecret(
        const std::string& client_id,
        const std::string& team_id,
        const std::string& key_id,
        const std::string& private_key,
        int duration,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        nlohmann::json payload;
        payload["clientId"] = client_id;
        payload["teamId"] = team_id;
        payload["keyId"] = key_id;
        payload["privateKey"] = private_key;
        payload["duration"] = duration;
        SendOptions opts;
        opts.method = "POST";
        opts.body = payload;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/settings/apple/generate-client-secret", std::move(opts));
    }
};

} // namespace bosbase

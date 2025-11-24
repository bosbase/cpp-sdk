#pragma once

#include <map>
#include <string>
#include <vector>

#include "bosbase/services/base.h"

namespace bosbase {

class BackupService : public BaseService {
public:
    explicit BackupService(BosBase& client) : BaseService(client) {}

    nlohmann::json getFullList(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/backups", std::move(opts));
    }

    nlohmann::json create(
        const std::string& basename = "",
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        auto params = query;
        params.emplace("basename", basename);
        SendOptions opts;
        opts.method = "POST";
        opts.query = params;
        opts.headers = headers;
        return client.send("/api/backups", std::move(opts));
    }

    nlohmann::json upload(
        const FileAttachment& file,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.files = { file };
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/backups/upload", std::move(opts));
    }

    void remove(
        const std::string& key,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        client.send("/api/backups/" + encodePathSegment(key), std::move(opts));
    }

    nlohmann::json restore(
        const std::string& key,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/backups/" + encodePathSegment(key) + "/restore", std::move(opts));
    }

    std::string getDownloadURL(const std::string& token, const std::string& key) {
        std::map<std::string, nlohmann::json> query = { {"token", token} };
        return client.buildUrl("/api/backups/" + encodePathSegment(key), query);
    }
};

} // namespace bosbase

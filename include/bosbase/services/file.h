#pragma once

#include <map>
#include <optional>
#include <string>

#include "bosbase/services/base.h"

namespace bosbase {

class FileService : public BaseService {
public:
    explicit FileService(BosBase& client) : BaseService(client) {}

    std::string getURL(
        const nlohmann::json& record,
        const std::string& filename,
        const std::optional<std::string>& thumb = std::nullopt,
        const std::optional<std::string>& token = std::nullopt,
        const std::optional<bool>& download = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {}) {
        std::map<std::string, nlohmann::json> params = query;
        if (thumb) params["thumb"] = *thumb;
        if (token) params["token"] = *token;
        if (download) params["download"] = *download;

        auto record_id = record.value("id", std::string{});
        auto collection_id = record.value("collectionId", std::string{});
        auto collection_name = record.value("collectionName", std::string{});
        if (collection_name.empty() && record.contains("@collectionName")) {
            collection_name = record["@collectionName"].get<std::string>();
        }
        std::string collection = !collection_name.empty() ? collection_name : collection_id;
        std::string path = "/api/files/" + encodePathSegment(collection) + "/" + encodePathSegment(record_id) + "/" + encodePathSegment(filename);
        return client.buildUrl(path, params);
    }

    nlohmann::json getToken(
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {}) {
        SendOptions opts;
        opts.method = "POST";
        opts.query = query;
        opts.headers = headers;
        return client.send("/api/files/token", std::move(opts));
    }
};

} // namespace bosbase

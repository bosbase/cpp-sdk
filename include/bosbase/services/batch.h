#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bosbase/services/base.h"

namespace bosbase {

class SubBatchService;

struct QueuedBatchRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    nlohmann::json body;
    std::vector<FileAttachment> files;
};

class BatchService : public BaseService {
public:
    explicit BatchService(BosBase& client) : BaseService(client) {}

    SubBatchService& collection(const std::string& collection);

    void queueRequest(
        const std::string& method,
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const nlohmann::json& body = {},
        const std::vector<FileAttachment>& files = {});

    nlohmann::json send(
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

private:
    std::vector<QueuedBatchRequest> requests_;
    std::map<std::string, std::unique_ptr<SubBatchService>> collections_;
};

class SubBatchService {
public:
    SubBatchService(BatchService& batch, const std::string& collection)
        : batch_(batch), collection_(collection) {}

    void create(
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::vector<FileAttachment>& files = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    void upsert(
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::vector<FileAttachment>& files = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    void update(
        const std::string& record_id,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::vector<FileAttachment>& files = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    void remove(
        const std::string& record_id,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {});

private:
    BatchService& batch_;
    std::string collection_;

    std::string collectionUrl() const {
        return "/api/collections/" + encodePathSegment(collection_) + "/records";
    }
};

inline SubBatchService& BatchService::collection(const std::string& collection) {
    auto it = collections_.find(collection);
    if (it == collections_.end()) {
        auto ptr = std::make_unique<SubBatchService>(*this, collection);
        auto& ref = *ptr;
        collections_[collection] = std::move(ptr);
        return ref;
    }
    return *it->second;
}

inline void BatchService::queueRequest(
    const std::string& method,
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    const nlohmann::json& body,
    const std::vector<FileAttachment>& files) {
    requests_.push_back(QueuedBatchRequest{method, url, headers, body.is_null() ? nlohmann::json::object() : body, files});
}

inline nlohmann::json BatchService::send(
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body.is_null() ? nlohmann::json::object() : body;
    payload["requests"] = nlohmann::json::array();
    std::vector<FileAttachment> attachments;

    for (size_t i = 0; i < requests_.size(); ++i) {
        const auto& req = requests_[i];
        nlohmann::json item;
        item["method"] = req.method;
        item["url"] = req.url;
        item["headers"] = req.headers;
        item["body"] = req.body;
        payload["requests"].push_back(item);

        for (const auto& file : req.files) {
            FileAttachment copy = file;
            copy.field = "requests." + std::to_string(i) + "." + copy.field;
            attachments.push_back(copy);
        }
    }

    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    opts.files = attachments;
    auto response = client.send("/api/batch", std::move(opts));
    requests_.clear();
    return response;
}

inline void SubBatchService::create(
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::vector<FileAttachment>& files,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    auto url = buildRelativeUrl(collectionUrl(), params);
    batch_.queueRequest("POST", url, {}, body, files);
}

inline void SubBatchService::upsert(
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::vector<FileAttachment>& files,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    auto url = buildRelativeUrl(collectionUrl(), params);
    batch_.queueRequest("PUT", url, {}, body, files);
}

inline void SubBatchService::update(
    const std::string& record_id,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::vector<FileAttachment>& files,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    auto url = buildRelativeUrl(collectionUrl() + "/" + encodePathSegment(record_id), params);
    batch_.queueRequest("PATCH", url, {}, body, files);
}

inline void SubBatchService::remove(
    const std::string& record_id,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query) {
    auto url = buildRelativeUrl(collectionUrl() + "/" + encodePathSegment(record_id), query);
    batch_.queueRequest("DELETE", url, {}, body, {});
}

} // namespace bosbase

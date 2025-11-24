#include "bosbase/services/base.h"
#include "bosbase/client.h"

#include <stdexcept>

namespace bosbase {

nlohmann::json BaseCrudService::getList(
    int page,
    int per_page,
    bool skip_total,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& filter,
    const std::optional<std::string>& sort,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto params = query;
    params.emplace("page", page);
    params.emplace("perPage", per_page);
    params.emplace("skipTotal", skip_total);
    if (filter) params.emplace("filter", *filter);
    if (sort) params.emplace("sort", *sort);
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions options;
    options.query = params;
    options.headers = headers;
    return client.send(baseCrudPath(), std::move(options));
}

nlohmann::json BaseCrudService::getOne(
    const std::string& record_id,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    if (record_id.empty()) {
        throw ClientResponseError(client.buildUrl(baseCrudPath() + "/"), 404, {
            {"code", 404},
            {"message", "Missing required record id."},
            {"data", nlohmann::json::object()}
        });
    }
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions options;
    options.query = params;
    options.headers = headers;
    return client.send(baseCrudPath() + "/" + encodePathSegment(record_id), std::move(options));
}

nlohmann::json BaseCrudService::getFirstListItem(
    const std::string& filter,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto data = getList(1, 1, true, query, headers, filter, std::nullopt, expand, fields);
    auto items_it = data.find("items");
    if (items_it == data.end() || !items_it->is_array() || items_it->empty()) {
        throw ClientResponseError({}, 404, {
            {"code", 404},
            {"message", "The requested resource wasn't found."},
            {"data", nlohmann::json::object()}
        });
    }
    return (*items_it)[0];
}

nlohmann::json BaseCrudService::getFullList(
    int batch,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& filter,
    const std::optional<std::string>& sort,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    if (batch <= 0) {
        throw std::invalid_argument("batch must be > 0");
    }
    nlohmann::json result = nlohmann::json::array();
    int page = 1;
    while (true) {
        auto data = getList(page, batch, true, query, headers, filter, sort, expand, fields);
        auto items = data.value("items", nlohmann::json::array());
        for (const auto& item : items) {
            result.push_back(item);
        }
        if (items.empty() || items.size() < data.value("perPage", batch)) {
            break;
        }
        ++page;
    }
    return result;
}

nlohmann::json BaseCrudService::create(
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::vector<FileAttachment>& files,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions options;
    options.method = "POST";
    options.body = body;
    options.query = params;
    options.headers = headers;
    options.files = files;
    return client.send(baseCrudPath(), std::move(options));
}

nlohmann::json BaseCrudService::update(
    const std::string& record_id,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::vector<FileAttachment>& files,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions options;
    options.method = "PATCH";
    options.body = body;
    options.query = params;
    options.headers = headers;
    options.files = files;
    return client.send(baseCrudPath() + "/" + encodePathSegment(record_id), std::move(options));
}

void BaseCrudService::remove(
    const std::string& record_id,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    SendOptions options;
    options.method = "DELETE";
    options.body = body;
    options.query = query;
    options.headers = headers;
    client.send(baseCrudPath() + "/" + encodePathSegment(record_id), std::move(options));
}

} // namespace bosbase

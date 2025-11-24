#pragma once

#include <map>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "bosbase/error.h"
#include "bosbase/request.h"
#include "bosbase/utils.h"

namespace bosbase {

class BosBase;

class BaseService {
public:
    explicit BaseService(BosBase& client) : client(client) {}
    virtual ~BaseService() = default;

protected:
    BosBase& client;
};

class BaseCrudService : public BaseService {
public:
    using BaseService::BaseService;
    virtual std::string baseCrudPath() const = 0;

    nlohmann::json getList(
        int page = 1,
        int per_page = 30,
        bool skip_total = false,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& filter = std::nullopt,
        const std::optional<std::string>& sort = std::nullopt,
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    nlohmann::json getOne(
        const std::string& record_id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    nlohmann::json getFirstListItem(
        const std::string& filter,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    nlohmann::json getFullList(
        int batch = 500,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& filter = std::nullopt,
        const std::optional<std::string>& sort = std::nullopt,
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    nlohmann::json create(
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::vector<FileAttachment>& files = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    nlohmann::json update(
        const std::string& record_id,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::vector<FileAttachment>& files = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    void remove(
        const std::string& record_id,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});
};

} // namespace bosbase

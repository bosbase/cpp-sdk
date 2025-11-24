#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "bosbase/auth_store.h"
#include "bosbase/error.h"
#include "bosbase/request.h"
#include "bosbase/types.h"
#include "bosbase/utils.h"

namespace bosbase {

class RecordService;
class CollectionService;
class FileService;
class LogService;
class RealtimeService;
class PubSubService;
class SettingsService;
class HealthService;
class BackupService;
class CronService;
class VectorService;
class LangChaingoService;
class LLMDocumentService;
class CacheService;
class GraphQLService;
class BatchService;

class BosBase {
public:
    explicit BosBase(const std::string& base_url = "/", std::shared_ptr<AuthStore> auth_store = nullptr, const std::string& lang = "en-US");
    ~BosBase();

    BosBase(const BosBase&) = delete;
    BosBase& operator=(const BosBase&) = delete;
    BosBase(BosBase&&) noexcept;
    BosBase& operator=(BosBase&&) noexcept;

    nlohmann::json send(const std::string& path, SendOptions options = {});

    std::string buildUrl(const std::string& path, const std::map<std::string, nlohmann::json>& query = {}) const;
    std::string filter(const std::string& expr, const std::map<std::string, nlohmann::json>& params = {}) const;

    std::shared_ptr<AuthStore> authStore() const { return auth_store_; }

    RecordService& collection(const std::string& id_or_name);
    std::unique_ptr<BatchService> createBatch();

    std::unique_ptr<CollectionService> collections;
    std::unique_ptr<FileService> files;
    std::unique_ptr<LogService> logs;
    std::unique_ptr<RealtimeService> realtime;
    std::unique_ptr<PubSubService> pubsub;
    std::unique_ptr<SettingsService> settings;
    std::unique_ptr<HealthService> health;
    std::unique_ptr<BackupService> backups;
    std::unique_ptr<CronService> crons;
    std::unique_ptr<VectorService> vectors;
    std::unique_ptr<LangChaingoService> langchaingo;
    std::unique_ptr<LLMDocumentService> llmDocuments;
    std::unique_ptr<CacheService> caches;
    std::unique_ptr<GraphQLService> graphql;

    BeforeSendHook beforeSend;
    AfterSendHook afterSend;

    const std::string& baseUrl() const { return base_url_; }
    const std::string& language() const { return lang_; }

private:
    std::string base_url_;
    std::string lang_;
    std::shared_ptr<AuthStore> auth_store_;
    std::map<std::string, std::unique_ptr<RecordService>> record_services_;
    std::recursive_mutex mutex_;

    friend class RealtimeService;
};

} // namespace bosbase

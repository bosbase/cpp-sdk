#include "bosbase/client.h"

#include <curl/curl.h>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "bosbase/services/backup.h"
#include "bosbase/services/batch.h"
#include "bosbase/services/cache.h"
#include "bosbase/services/collection.h"
#include "bosbase/services/cron.h"
#include "bosbase/services/file.h"
#include "bosbase/services/graphql.h"
#include "bosbase/services/health.h"
#include "bosbase/services/langchaingo.h"
#include "bosbase/services/llm_document.h"
#include "bosbase/services/log.h"
#include "bosbase/services/pubsub.h"
#include "bosbase/services/realtime.h"
#include "bosbase/services/record.h"
#include "bosbase/services/settings.h"
#include "bosbase/services/sql.h"
#include "bosbase/services/vector.h"

namespace bosbase {

namespace {
size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* stream = static_cast<std::string*>(userdata);
    size_t total = size * nmemb;
    stream->append(ptr, total);
    return total;
}

size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total = size * nitems;
    std::string header(buffer, total);
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
    auto colon = header.find(':');
    if (colon != std::string::npos) {
        std::string key = header.substr(0, colon);
        std::string value = header.substr(colon + 1);
        auto key_end = key.find_last_not_of(" \t\r\n");
        if (key_end != std::string::npos) key.erase(key_end + 1);
        auto val_start = value.find_first_not_of(" \t\r\n");
        if (val_start != std::string::npos) value.erase(0, val_start);
        auto val_end = value.find_last_not_of(" \t\r\n");
        if (val_end != std::string::npos) value.erase(val_end + 1);
        (*headers)[key] = value;
    }
    return total;
}
} // namespace

BosBase::BosBase(const std::string& base_url, std::shared_ptr<AuthStore> auth_store, const std::string& lang)
    : base_url_(base_url.empty() ? "/" : base_url), lang_(lang), auth_store_(std::move(auth_store)) {
    if (!auth_store_) {
        auth_store_ = std::make_shared<AuthStore>();
    }

    collections = std::make_unique<CollectionService>(*this);
    files = std::make_unique<FileService>(*this);
    logs = std::make_unique<LogService>(*this);
    realtime = std::make_unique<RealtimeService>(*this);
    pubsub = std::make_unique<PubSubService>(*this);
    settings = std::make_unique<SettingsService>(*this);
    health = std::make_unique<HealthService>(*this);
    backups = std::make_unique<BackupService>(*this);
    crons = std::make_unique<CronService>(*this);
    vectors = std::make_unique<VectorService>(*this);
    langchaingo = std::make_unique<LangChaingoService>(*this);
    llmDocuments = std::make_unique<LLMDocumentService>(*this);
    caches = std::make_unique<CacheService>(*this);
    graphql = std::make_unique<GraphQLService>(*this);
    sql = std::make_unique<SQLService>(*this);
}

BosBase::BosBase(BosBase&& other) noexcept
    : base_url_(std::move(other.base_url_)),
      lang_(std::move(other.lang_)),
      auth_store_(std::move(other.auth_store_)),
      record_services_(),
      collections(std::move(other.collections)),
      files(std::move(other.files)),
      logs(std::move(other.logs)),
      realtime(std::move(other.realtime)),
      pubsub(std::move(other.pubsub)),
      settings(std::move(other.settings)),
      health(std::move(other.health)),
      backups(std::move(other.backups)),
      crons(std::move(other.crons)),
      vectors(std::move(other.vectors)),
      langchaingo(std::move(other.langchaingo)),
      llmDocuments(std::move(other.llmDocuments)),
      caches(std::move(other.caches)),
      graphql(std::move(other.graphql)),
      sql(std::move(other.sql)),
      beforeSend(std::move(other.beforeSend)),
      afterSend(std::move(other.afterSend)) {
}

BosBase& BosBase::operator=(BosBase&& other) noexcept {
    if (this != &other) {
        base_url_ = std::move(other.base_url_);
        lang_ = std::move(other.lang_);
        auth_store_ = std::move(other.auth_store_);
        record_services_.clear();
        collections = std::move(other.collections);
        files = std::move(other.files);
        logs = std::move(other.logs);
        realtime = std::move(other.realtime);
        pubsub = std::move(other.pubsub);
        settings = std::move(other.settings);
        health = std::move(other.health);
        backups = std::move(other.backups);
        crons = std::move(other.crons);
        vectors = std::move(other.vectors);
        langchaingo = std::move(other.langchaingo);
        llmDocuments = std::move(other.llmDocuments);
        caches = std::move(other.caches);
        graphql = std::move(other.graphql);
        sql = std::move(other.sql);
        beforeSend = std::move(other.beforeSend);
        afterSend = std::move(other.afterSend);
    }
    return *this;
}

BosBase::~BosBase() = default;

nlohmann::json BosBase::send(const std::string& path, SendOptions options) {
    std::string url = buildUrl(path, options.query);

    if (beforeSend) {
        beforeSend(url, options);
    }
    url = buildUrl(path, options.query);

    std::map<std::string, std::string> headers = {
        {"Accept-Language", lang_},
        {"User-Agent", "bosbase-cpp-sdk/0.1.0"},
    };
    for (const auto& kv : options.headers) {
        headers[kv.first] = kv.second;
    }
    if (headers.find("Authorization") == headers.end() && auth_store_ && auth_store_->isValid()) {
        headers["Authorization"] = auth_store_->token();
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw ClientResponseError(url, 0, {{"message", "Failed to initialize curl"}});
    }

    std::string response_body;
    std::map<std::string, std::string> response_headers;
    struct curl_slist* curl_headers = nullptr;
    for (const auto& kv : headers) {
        std::string header_line = kv.first + ": " + kv.second;
        curl_headers = curl_slist_append(curl_headers, header_line.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, options.timeoutMs.value_or(30000));

    std::string method = options.method.empty() ? "GET" : options.method;
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());

    curl_mime* mime = nullptr;
    std::string body_str;

    if (!options.files.empty()) {
        mime = curl_mime_init(curl);

        // attach JSON payload
        curl_mimepart* json_part = curl_mime_addpart(mime);
        curl_mime_name(json_part, "@jsonPayload");
        std::string json_str = options.body.is_null() ? "{}" : options.body.dump();
        curl_mime_data(json_part, json_str.c_str(), CURL_ZERO_TERMINATED);

        for (const auto& file : options.files) {
            curl_mimepart* part = curl_mime_addpart(mime);
            curl_mime_name(part, file.field.c_str());
            curl_mime_filename(part, file.filename.c_str());
            curl_mime_data(part, reinterpret_cast<const char*>(file.data.data()), file.data.size());
            curl_mime_type(part, file.contentType.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    } else {
        body_str = options.body.is_null() ? "" : options.body.dump();
        if (method == "GET" || method == "HEAD") {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
            curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
        }
    }

    auto res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    if (curl_headers) {
        curl_slist_free_all(curl_headers);
    }
    if (mime) {
        curl_mime_free(mime);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw ClientResponseError(url, 0, {{"message", curl_easy_strerror(res)}}, res == CURLE_ABORTED_BY_CALLBACK);
    }

    nlohmann::json data;
    if (!response_body.empty()) {
        auto content_type_it = response_headers.find("Content-Type");
        if (content_type_it != response_headers.end() && content_type_it->second.find("application/json") != std::string::npos) {
            try {
                data = nlohmann::json::parse(response_body);
            } catch (...) {
                data = nlohmann::json::object();
            }
        } else {
            data = response_body;
        }
    }

    if (status >= 400) {
        throw ClientResponseError(url, status, data.is_object() ? data : nlohmann::json::object());
    }

    if (afterSend) {
        data = afterSend(status, response_headers, data);
    }

    return data;
}

std::string BosBase::buildUrl(const std::string& path, const std::map<std::string, nlohmann::json>& query) const {
    std::string base = base_url_;
    if (!base.empty() && base.back() != '/') {
        base += "/";
    }
    std::string rel = path;
    if (!rel.empty() && rel.front() == '/') {
        rel.erase(0, 1);
    }
    std::string url = base + rel;
    auto normalized = normalizeQuery(query);
    auto query_str = buildQuery(normalized);
    if (!query_str.empty()) {
        url += (url.find('?') == std::string::npos ? "?" : "&") + query_str;
    }
    return url;
}

std::string BosBase::filter(const std::string& expr, const std::map<std::string, nlohmann::json>& params) const {
    if (params.empty()) return expr;
    std::string result = expr;
    for (const auto& kv : params) {
        std::string placeholder = "{:" + kv.first + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            std::string replacement;
            if (kv.second.is_string()) {
                std::string safe = kv.second.get<std::string>();
                std::string escaped;
                for (char c : safe) {
                    if (c == '\'') escaped += "\\'";
                    else escaped += c;
                }
                replacement = "'" + escaped + "'";
            } else if (kv.second.is_boolean()) {
                replacement = kv.second.get<bool>() ? "true" : "false";
            } else if (kv.second.is_null()) {
                replacement = "null";
            } else {
                replacement = kv.second.dump();
            }
            result.replace(pos, placeholder.size(), replacement);
            pos += replacement.size();
        }
    }
    return result;
}

RecordService& BosBase::collection(const std::string& id_or_name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = record_services_.find(id_or_name);
    if (it == record_services_.end()) {
        auto ptr = std::make_unique<RecordService>(*this, id_or_name);
        auto& ref = *ptr;
        record_services_[id_or_name] = std::move(ptr);
        return ref;
    }
    return *it->second;
}

std::unique_ptr<BatchService> BosBase::createBatch() {
    return std::make_unique<BatchService>(*this);
}

} // namespace bosbase

#pragma once

#include <optional>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace bosbase {

struct VectorDocument {
    std::vector<float> vector;
    std::optional<std::string> id;
    std::optional<nlohmann::json> metadata;
    std::optional<std::string> content;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["vector"] = vector;
        if (id) payload["id"] = *id;
        if (metadata) payload["metadata"] = *metadata;
        if (content) payload["content"] = *content;
        return payload;
    }
};

struct VectorSearchOptions {
    std::vector<float> queryVector;
    std::optional<int> limit;
    std::optional<nlohmann::json> filter;
    std::optional<float> minScore;
    std::optional<float> maxDistance;
    std::optional<bool> includeDistance;
    std::optional<bool> includeContent;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["queryVector"] = queryVector;
        if (limit) payload["limit"] = *limit;
        if (filter) payload["filter"] = *filter;
        if (minScore) payload["minScore"] = *minScore;
        if (maxDistance) payload["maxDistance"] = *maxDistance;
        if (includeDistance) payload["includeDistance"] = *includeDistance;
        if (includeContent) payload["includeContent"] = *includeContent;
        return payload;
    }
};

struct VectorBatchInsertOptions {
    std::vector<VectorDocument> documents;
    std::optional<bool> skipDuplicates;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["documents"] = nlohmann::json::array();
        for (const auto& doc : documents) {
            payload["documents"].push_back(doc.toJson());
        }
        if (skipDuplicates) payload["skipDuplicates"] = *skipDuplicates;
        return payload;
    }
};

struct VectorCollectionConfig {
    std::optional<int> dimension;
    std::optional<std::string> distance;
    std::optional<nlohmann::json> options;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        if (dimension) payload["dimension"] = *dimension;
        if (distance) payload["distance"] = *distance;
        if (options) payload["options"] = *options;
        return payload;
    }
};

struct LangChaingoModelConfig {
    std::optional<std::string> provider;
    std::optional<std::string> model;
    std::optional<std::string> apiKey;
    std::optional<std::string> baseUrl;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        if (provider) payload["provider"] = *provider;
        if (model) payload["model"] = *model;
        if (apiKey) payload["apiKey"] = *apiKey;
        if (baseUrl) payload["baseUrl"] = *baseUrl;
        return payload;
    }
};

struct LangChaingoCompletionMessage {
    std::string content;
    std::optional<std::string> role;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["content"] = content;
        if (role) payload["role"] = *role;
        return payload;
    }
};

struct LangChaingoCompletionRequest {
    std::optional<LangChaingoModelConfig> model;
    std::optional<std::string> prompt;
    std::vector<LangChaingoCompletionMessage> messages;
    std::optional<double> temperature;
    std::optional<int> maxTokens;
    std::optional<double> topP;
    std::optional<int> candidateCount;
    std::optional<std::vector<std::string>> stop;
    std::optional<bool> jsonResponse;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        if (model) payload["model"] = model->toJson();
        if (prompt) payload["prompt"] = *prompt;
        if (!messages.empty()) {
            payload["messages"] = nlohmann::json::array();
            for (const auto& msg : messages) payload["messages"].push_back(msg.toJson());
        }
        if (temperature) payload["temperature"] = *temperature;
        if (maxTokens) payload["maxTokens"] = *maxTokens;
        if (topP) payload["topP"] = *topP;
        if (candidateCount) payload["candidateCount"] = *candidateCount;
        if (stop) payload["stop"] = *stop;
        if (jsonResponse) payload["json"] = *jsonResponse;
        return payload;
    }
};

struct LangChaingoRAGFilters {
    std::optional<std::map<std::string, std::string>> where;
    std::optional<std::map<std::string, std::string>> whereDocument;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        if (where) payload["where"] = *where;
        if (whereDocument) payload["whereDocument"] = *whereDocument;
        return payload;
    }
};

struct LangChaingoRAGRequest {
    std::string collection;
    std::string question;
    std::optional<LangChaingoModelConfig> model;
    std::optional<int> topK;
    std::optional<double> scoreThreshold;
    std::optional<LangChaingoRAGFilters> filters;
    std::optional<std::string> promptTemplate;
    std::optional<bool> returnSources;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["collection"] = collection;
        payload["question"] = question;
        if (model) payload["model"] = model->toJson();
        if (topK) payload["topK"] = *topK;
        if (scoreThreshold) payload["scoreThreshold"] = *scoreThreshold;
        if (filters) payload["filters"] = filters->toJson();
        if (promptTemplate) payload["promptTemplate"] = *promptTemplate;
        if (returnSources) payload["returnSources"] = *returnSources;
        return payload;
    }
};

struct LangChaingoDocumentQueryRequest {
    std::string collection;
    std::string query;
    std::optional<LangChaingoModelConfig> model;
    std::optional<int> topK;
    std::optional<double> scoreThreshold;
    std::optional<LangChaingoRAGFilters> filters;
    std::optional<std::string> promptTemplate;
    std::optional<bool> returnSources;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["collection"] = collection;
        payload["query"] = query;
        if (model) payload["model"] = model->toJson();
        if (topK) payload["topK"] = *topK;
        if (scoreThreshold) payload["scoreThreshold"] = *scoreThreshold;
        if (filters) payload["filters"] = filters->toJson();
        if (promptTemplate) payload["promptTemplate"] = *promptTemplate;
        if (returnSources) payload["returnSources"] = *returnSources;
        return payload;
    }
};

struct LangChaingoSQLRequest {
    std::string query;
    std::optional<std::vector<std::string>> tables;
    std::optional<int> topK;
    std::optional<LangChaingoModelConfig> model;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["query"] = query;
        if (tables) payload["tables"] = *tables;
        if (topK) payload["topK"] = *topK;
        if (model) payload["model"] = model->toJson();
        return payload;
    }
};

struct LLMDocument {
    std::string id;
    std::string content;
    std::optional<nlohmann::json> metadata;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["id"] = id;
        payload["content"] = content;
        if (metadata) payload["metadata"] = *metadata;
        return payload;
    }

    static LLMDocument fromJson(const nlohmann::json& data) {
        LLMDocument doc;
        if (data.contains("id")) doc.id = data["id"].get<std::string>();
        if (data.contains("content")) doc.content = data["content"].get<std::string>();
        if (data.contains("metadata")) doc.metadata = data["metadata"];
        return doc;
    }
};

struct LLMDocumentUpdate {
    std::optional<std::string> content;
    std::optional<nlohmann::json> metadata;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        if (content) payload["content"] = *content;
        if (metadata) payload["metadata"] = *metadata;
        return payload;
    }
};

struct LLMQueryOptions {
    std::string query;
    std::optional<int> topK;
    std::optional<nlohmann::json> filter;
    std::optional<bool> includeDocument;

    nlohmann::json toJson() const {
        nlohmann::json payload;
        payload["query"] = query;
        if (topK) payload["topK"] = *topK;
        if (filter) payload["filter"] = *filter;
        if (includeDocument) payload["includeDocument"] = *includeDocument;
        return payload;
    }
};

struct SQLExecuteRequest {
    std::string query;

    nlohmann::json toJson() const {
        return {{"query", query}};
    }
};

struct SQLExecuteResponse {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    std::optional<int64_t> rowsAffected;

    static SQLExecuteResponse fromJson(const nlohmann::json& data) {
        SQLExecuteResponse res;
        if (data.is_object()) {
            if (data.contains("columns") && data["columns"].is_array()) {
                res.columns = data["columns"].get<std::vector<std::string>>();
            }
            if (data.contains("rows") && data["rows"].is_array()) {
                res.rows = data["rows"].get<std::vector<std::vector<std::string>>>();
            }
            if (data.contains("rowsAffected") && !data["rowsAffected"].is_null()) {
                res.rowsAffected = data["rowsAffected"].get<int64_t>();
            }
        }
        return res;
    }
};

} // namespace bosbase

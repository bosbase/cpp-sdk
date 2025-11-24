#include "bosbase/services/record.h"

#include <utility>
#include "bosbase/client.h"
#include "bosbase/services/realtime.h"
#include "bosbase/utils.h"

namespace bosbase {

std::string RecordService::baseCollectionPath() const {
    return "/api/collections/" + encodePathSegment(collection_id_or_name_);
}

std::string RecordService::baseCrudPath() const {
    return baseCollectionPath() + "/records";
}

std::function<void()> RecordService::subscribe(
    const std::string& topic,
    RecordSubscriptionCallback callback,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    if (!client.realtime) {
        throw std::runtime_error("Realtime service is not initialized");
    }
    std::string full_topic = collection_id_or_name_ + "/" + topic;
    return client.realtime->subscribe(full_topic, std::move(callback), query, headers);
}

void RecordService::unsubscribe(const std::optional<std::string>& topic) {
    if (!client.realtime) return;
    if (topic) {
        client.realtime->unsubscribe(collection_id_or_name_ + "/" + *topic);
    } else {
        client.realtime->unsubscribeByPrefix(collection_id_or_name_);
    }
}

int RecordService::getCount(
    const std::optional<std::string>& filter,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    auto params = query;
    if (filter) params.emplace("filter", *filter);
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions opts;
    opts.query = params;
    opts.headers = headers;
    auto res = client.send(baseCrudPath() + "/count", std::move(opts));
    return res.value("count", 0);
}

nlohmann::json RecordService::listAuthMethods(
    const std::optional<std::string>& fields,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    auto params = query;
    params.emplace("fields", fields.value_or("mfa,otp,password,oauth2"));
    SendOptions opts;
    opts.query = params;
    opts.headers = headers;
    return client.send(baseCollectionPath() + "/auth-methods", std::move(opts));
}

nlohmann::json RecordService::authWithPassword(
    const std::string& identity,
    const std::string& password,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["identity"] = identity;
    payload["password"] = password;
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = params;
    opts.headers = headers;
    auto res = client.send(baseCollectionPath() + "/auth-with-password", std::move(opts));
    return handleAuthResponse(res);
}

nlohmann::json RecordService::authWithOTP(
    const std::string& otp_id,
    const std::string& password,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["otpId"] = otp_id;
    payload["password"] = password;
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = params;
    opts.headers = headers;
    auto res = client.send(baseCollectionPath() + "/auth-with-otp", std::move(opts));
    return handleAuthResponse(res);
}

nlohmann::json RecordService::authWithOAuth2Code(
    const std::string& provider,
    const std::string& code,
    const std::string& code_verifier,
    const std::string& redirect_url,
    const std::optional<nlohmann::json>& create_data,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    nlohmann::json payload = body;
    payload["provider"] = provider;
    payload["code"] = code;
    payload["codeVerifier"] = code_verifier;
    payload["redirectURL"] = redirect_url;
    if (create_data) payload["createData"] = *create_data;
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = params;
    opts.headers = headers;
    auto res = client.send(baseCollectionPath() + "/auth-with-oauth2", std::move(opts));
    return handleAuthResponse(res);
}

nlohmann::json RecordService::authRefresh(
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions opts;
    opts.method = "POST";
    opts.body = body;
    opts.query = params;
    opts.headers = headers;
    auto res = client.send(baseCollectionPath() + "/auth-refresh", std::move(opts));
    return handleAuthResponse(res);
}

void RecordService::requestPasswordReset(
    const std::string& email,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["email"] = email;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    client.send(baseCollectionPath() + "/request-password-reset", std::move(opts));
}

void RecordService::confirmPasswordReset(
    const std::string& token,
    const std::string& password,
    const std::string& password_confirm,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["token"] = token;
    payload["password"] = password;
    payload["passwordConfirm"] = password_confirm;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    client.send(baseCollectionPath() + "/confirm-password-reset", std::move(opts));
}

void RecordService::requestVerification(
    const std::string& email,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["email"] = email;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    client.send(baseCollectionPath() + "/request-verification", std::move(opts));
}

void RecordService::confirmVerification(
    const std::string& token,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["token"] = token;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    client.send(baseCollectionPath() + "/confirm-verification", std::move(opts));
    markVerified(token);
}

void RecordService::requestEmailChange(
    const std::string& new_email,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["newEmail"] = new_email;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    client.send(baseCollectionPath() + "/request-email-change", std::move(opts));
}

void RecordService::confirmEmailChange(
    const std::string& token,
    const std::string& password,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["token"] = token;
    payload["password"] = password;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    client.send(baseCollectionPath() + "/confirm-email-change", std::move(opts));
    clearIfSameToken(token);
}

nlohmann::json RecordService::requestOTP(
    const std::string& email,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    nlohmann::json payload = body;
    payload["email"] = email;
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = query;
    opts.headers = headers;
    return client.send(baseCollectionPath() + "/request-otp", std::move(opts));
}

nlohmann::json RecordService::listExternalAuths(
    const std::string& record_id,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    SendOptions opts;
    opts.query = query;
    opts.headers = headers;
    return client.send(baseCollectionPath() + "/records/" + encodePathSegment(record_id) + "/external-auths", std::move(opts));
}

void RecordService::unlinkExternalAuth(
    const std::string& record_id,
    const std::string& provider,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    SendOptions opts;
    opts.method = "DELETE";
    opts.query = query;
    opts.headers = headers;
    client.send(baseCollectionPath() + "/records/" + encodePathSegment(record_id) + "/external-auths/" + encodePathSegment(provider), std::move(opts));
}

BosBase RecordService::impersonate(
    const std::string& record_id,
    int duration,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    nlohmann::json payload = body;
    payload["duration"] = duration;
    auto params = query;
    if (expand) params.emplace("expand", *expand);
    if (fields) params.emplace("fields", *fields);
    SendOptions opts;
    opts.method = "POST";
    opts.body = payload;
    opts.query = params;
    auto enriched_headers = headers;
    if (enriched_headers.find("Authorization") == enriched_headers.end() && client.authStore()->isValid()) {
        enriched_headers["Authorization"] = client.authStore()->token();
    }
    opts.headers = enriched_headers;

    BosBase new_client(client.baseUrl(), std::make_shared<AuthStore>(), client.language());
    auto res = new_client.send(baseCollectionPath() + "/impersonate/" + encodePathSegment(record_id), std::move(opts));
    new_client.authStore()->save(res.value("token", ""), res.value("record", nlohmann::json::object()));
    return std::move(new_client);
}

nlohmann::json RecordService::update(
    const std::string& record_id,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::vector<FileAttachment>& files,
    const std::map<std::string, std::string>& headers,
    const std::optional<std::string>& expand,
    const std::optional<std::string>& fields) {
    auto item = BaseCrudService::update(record_id, body, query, files, headers, expand, fields);
    maybeUpdateAuthRecord(item);
    return item;
}

void RecordService::remove(
    const std::string& record_id,
    const nlohmann::json& body,
    const std::map<std::string, nlohmann::json>& query,
    const std::map<std::string, std::string>& headers) {
    BaseCrudService::remove(record_id, body, query, headers);
    if (isAuthRecord(record_id)) {
        client.authStore()->clear();
    }
}

nlohmann::json RecordService::handleAuthResponse(const nlohmann::json& data) {
    auto token = data.value("token", std::string{});
    auto record = data.contains("record") ? data.at("record") : nlohmann::json{};
    if (!token.empty() && !record.is_null()) {
        client.authStore()->save(token, record);
    }
    return data;
}

void RecordService::maybeUpdateAuthRecord(const nlohmann::json& item) {
    auto current = client.authStore()->record();
    if (current.is_null()) return;
    auto id = current.value("id", std::string{});
    if (id != item.value("id", std::string{})) return;
    auto collectionId = current.value("collectionId", std::string{});
    auto collectionName = current.value("collectionName", std::string{});
    if (collectionId != collection_id_or_name_ && collectionName != collection_id_or_name_) return;

    nlohmann::json merged = current;
    for (auto it = item.begin(); it != item.end(); ++it) {
        merged[it.key()] = it.value();
    }
    if (current.contains("expand") && item.contains("expand")) {
        nlohmann::json expand = current["expand"];
        for (auto it = item["expand"].begin(); it != item["expand"].end(); ++it) {
            expand[it.key()] = it.value();
        }
        merged["expand"] = expand;
    }
    client.authStore()->save(client.authStore()->token(), merged);
}

bool RecordService::isAuthRecord(const std::string& record_id) const {
    auto current = client.authStore()->record();
    if (current.is_null()) return false;
    return current.value("id", std::string{}) == record_id &&
        (current.value("collectionId", std::string{}) == collection_id_or_name_ ||
         current.value("collectionName", std::string{}) == collection_id_or_name_);
}

void RecordService::markVerified(const std::string& token) {
    auto current = client.authStore()->record();
    if (current.is_null()) return;
    auto payload = decodeTokenPayload(token);
    if (payload.is_null()) return;
    if (current.value("id", std::string{}) == payload.value("id", std::string{}) &&
        current.value("collectionId", std::string{}) == payload.value("collectionId", std::string{}) &&
        !current.value("verified", false)) {
        current["verified"] = true;
        client.authStore()->save(client.authStore()->token(), current);
    }
}

void RecordService::clearIfSameToken(const std::string& token) {
    auto current = client.authStore()->record();
    if (current.is_null()) return;
    auto payload = decodeTokenPayload(token);
    if (payload.is_null()) return;
    if (current.value("id", std::string{}) == payload.value("id", std::string{}) &&
        current.value("collectionId", std::string{}) == payload.value("collectionId", std::string{})) {
        client.authStore()->clear();
    }
}

nlohmann::json RecordService::decodeTokenPayload(const std::string& token) {
    auto first = token.find('.');
    auto second = token.find('.', first + 1);
    if (first == std::string::npos || second == std::string::npos) return {};
    auto payload = token.substr(first + 1, second - first - 1);
    auto decoded = base64UrlDecode(payload);
    if (decoded.empty()) return {};
    try {
        return nlohmann::json::parse(decoded);
    } catch (...) {
        return {};
    }
}

} // namespace bosbase

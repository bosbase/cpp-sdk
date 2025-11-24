#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "bosbase/services/base.h"

namespace bosbase {

using RecordSubscriptionCallback = std::function<void(const nlohmann::json&)>;

class RecordService : public BaseCrudService {
public:
    RecordService(BosBase& client, std::string collection)
        : BaseCrudService(client), collection_id_or_name_(std::move(collection)) {}

    std::string baseCollectionPath() const;
    std::string baseCrudPath() const override;

    // realtime
    std::function<void()> subscribe(
        const std::string& topic,
        RecordSubscriptionCallback callback,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});
    void unsubscribe(const std::optional<std::string>& topic = std::nullopt);

    // helpers
    int getCount(
        const std::optional<std::string>& filter = std::nullopt,
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    nlohmann::json listAuthMethods(
        const std::optional<std::string>& fields = std::nullopt,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    nlohmann::json authWithPassword(
        const std::string& identity,
        const std::string& password,
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    nlohmann::json authWithOTP(
        const std::string& otp_id,
        const std::string& password,
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    nlohmann::json authWithOAuth2Code(
        const std::string& provider,
        const std::string& code,
        const std::string& code_verifier,
        const std::string& redirect_url,
        const std::optional<nlohmann::json>& create_data = std::nullopt,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    nlohmann::json authRefresh(
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    void requestPasswordReset(
        const std::string& email,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    void confirmPasswordReset(
        const std::string& token,
        const std::string& password,
        const std::string& password_confirm,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    void requestVerification(
        const std::string& email,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    void confirmVerification(
        const std::string& token,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    void requestEmailChange(
        const std::string& new_email,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    void confirmEmailChange(
        const std::string& token,
        const std::string& password,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    nlohmann::json requestOTP(
        const std::string& email,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    nlohmann::json listExternalAuths(
        const std::string& record_id,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    void unlinkExternalAuth(
        const std::string& record_id,
        const std::string& provider,
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {});

    BosBase impersonate(
        const std::string& record_id,
        int duration,
        const nlohmann::json& body = {},
        const std::map<std::string, nlohmann::json>& query = {},
        const std::map<std::string, std::string>& headers = {},
        const std::optional<std::string>& expand = std::nullopt,
        const std::optional<std::string>& fields = std::nullopt);

    // override to sync auth record
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

private:
    std::string collection_id_or_name_;

    nlohmann::json handleAuthResponse(const nlohmann::json& data);
    void maybeUpdateAuthRecord(const nlohmann::json& item);
    bool isAuthRecord(const std::string& record_id) const;
    void markVerified(const std::string& token);
    void clearIfSameToken(const std::string& token);
    static nlohmann::json decodeTokenPayload(const std::string& token);
};

} // namespace bosbase

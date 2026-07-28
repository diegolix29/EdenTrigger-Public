// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_api.h"
#include "common/nextendo/nextendo_account.h"
#include "common/nextendo/nextendo_endpoint.h"
#include "common/nextendo/nextendo_crypto_utils.h"
#include "common/nextendo/nextendo_oauth_listener.h"
#include "common/nextendo/nextendo_platform_utils.h"
#include "common/logging.h"
#include "common/httplib.h"
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <thread>
#include <chrono>

namespace Common::Nextendo {

using json = nlohmann::json;

std::function<void()> NextendoApi::SessionRevoked;

std::string NextendoApi::GetBaseUrl() {
    return NextendoEndpoint::GetBaseUrl();
}

std::string NextendoApi::GetSiteUrl() {
    const char* url = std::getenv("NEXTENDO_SITE");
    if (url && strlen(url) > 0) {
        std::string result = url;
        if (!result.empty() && result.back() == '/') {
            result.pop_back();
        }
        return result;
    }
    return "https://nextendo.network";
}

bool NextendoApi::HealIfRejected(int statusCode) {
    if (statusCode != 401 || NextendoAccount::GetNexToken().empty()) {
        return false;
    }

    LOG_WARNING(Common, "[Nextendo] Token rejected by server (401) — local session purged. "
               "Reconnection required.");
    NextendoAccount::Clear();
    if (SessionRevoked) {
        try {
            SessionRevoked();
        } catch (...) {
            // UI not ready yet
        }
    }
    return true;
}

std::vector<NextendoApi::HistoryItem> NextendoApi::SyncHistory(const std::vector<NextendoApi::HistoryItem>& local) {
    std::vector<NextendoApi::HistoryItem> result;
    try {
        json items = json::array();
        for (const auto& h : local) {
            items.push_back({
                {"title_id", h.title_id},
                {"name", h.name},
                {"icon", h.icon_base64},
                {"seconds", h.seconds},
                {"last_played", h.last_played}
            });
        }

        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(15);
        client.set_read_timeout(15);
        client.set_write_timeout(15);

        if (!NextendoAccount::GetNexToken().empty()) {
            client.set_bearer_token_auth(NextendoAccount::GetNexToken());
        }

        json payload = {{"history", items}};
        auto res = client.Put("/api/history", payload.dump(), "application/json");

        if (res && res->status == 200) {
            json response = json::parse(res->body);
            if (response.contains("history") && response["history"].is_array()) {
                for (const auto& item : response["history"]) {
                    NextendoApi::HistoryItem h;
                    h.title_id = item.value("title_id", "");
                    h.name = item.value("name", "");
                    h.icon_base64 = item.value("icon", "");
                    h.seconds = item.value("seconds", (int64_t)0);
                    h.last_played = item.value("last_played", "");
                    result.push_back(h);
                }
            }
        }
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] History sync failed: {}", ex.what());
    }
    return result;
}

std::string NextendoApi::GetOnlineRefusalReason() {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(8);
        client.set_read_timeout(8);
        client.set_write_timeout(8);

        auto res = client.Get("/api/online-status");
        if (!res || res->status != 200) {
            return "";
        }

        json response = json::parse(res->body);
        if (response.value("allow", false)) {
            return "";
        }
        return response.value("reason", "");
    } catch (...) {
        return "";
    }
}

std::tuple<std::string, std::vector<uint8_t>> NextendoApi::GetProfileSync() {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(15);
        client.set_read_timeout(15);
        client.set_write_timeout(15);

        if (!NextendoAccount::GetNexToken().empty()) {
            client.set_bearer_token_auth(NextendoAccount::GetNexToken());
        }

        auto res = client.Get("/api/profile");
        if (!res) {
            return {nullptr, {}};
        }

        if (res->status != 200) {
            HealIfRejected(res->status);
            return {nullptr, {}};
        }

        json response = json::parse(res->body);
        std::string name = response.value("username", "");

        std::vector<uint8_t> image;
        if (response.contains("profile") && response["profile"].is_object()) {
            auto& profile = response["profile"];
            if (profile.contains("image") && profile["image"].is_string()) {
                std::string b64 = profile["image"].get<std::string>();
                // Decode base64 (simplified - use proper base64 decoder in production)
                // For now, return empty
            }
        }

        return {name, image};
    } catch (...) {
        return {nullptr, {}};
    }
}

std::tuple<bool, std::string> NextendoApi::CreateGuest(const std::string& nickname) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(15);
        client.set_read_timeout(15);
        client.set_write_timeout(15);

        json payload = {{"username", nickname}};
        auto res = client.Post("/api/guest", payload.dump(), "application/json");

        if (!res) {
            return {false, "Failed to connect"};
        }

        json response = json::parse(res->body);
        if (res->status != 200) {
            return {false, response.value("error", "Error")};
        }

        std::string nexToken = response.value("nex_token", "");
        uint64_t pid = response["account"].value("pid", (uint64_t)0);
        std::string username = response["account"].value("username", nickname);
        std::string friendCode = response["account"].value("friend_code", "");

        if (pid == 0 || nexToken.empty()) {
            return {false, "Invalid server response"};
        }

        NextendoAccount::Save(pid, username, friendCode, nexToken, true);
        return {true, ""};
    } catch (const std::exception& ex) {
        return {false, ex.what()};
    }
}

std::optional<bool> NextendoApi::CheckNicknameAvailable(const std::string& nickname) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(8);
        client.set_read_timeout(8);
        client.set_write_timeout(8);

        std::string url = "/api/username-available?username=" + nickname;
        auto res = client.Get(url);

        if (!res) {
            return std::nullopt;
        }

        json response = json::parse(res->body);
        if (response.contains("available")) {
            return response["available"].get<bool>();
        }
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::tuple<NextendoApi::BetaConfig, bool> NextendoApi::GetBetaConfig() {
    NextendoApi::BetaConfig config;
    try {
        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(8);
        client.set_read_timeout(8);
        client.set_write_timeout(8);

        std::string url = "/api/beta-config?channel=" + std::string(ReleaseChannel);
        auto res = client.Get(url);

        if (!res || res->status != 200) {
            return {config, false};
        }

        json response = json::parse(res->body);
        config.online_enabled = response.value("online_enabled", false);
        config.min_app_version = response.value("min_app_version", "0.0.0");
        config.message_en = response.value("message_en", "");
        config.message_fr = response.value("message_fr", "");
        config.force_update_url = response.value("force_update_url", "");

        return {config, true};
    } catch (...) {
        return {config, false};
    }
}

void NextendoApi::TouchSession() {
    if (NextendoAccount::GetNexToken().empty()) {
        return;
    }

    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());
        client.Post("/api/nex-session", "{}", "application/json");
    } catch (...) {
        // Best-effort
    }
}

std::tuple<std::vector<NextendoApi::Friend>, std::vector<NextendoApi::Friend>> NextendoApi::GetSocial() {
    std::vector<NextendoApi::Friend> friends;
    std::vector<NextendoApi::Friend> requests;
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        auto res = client.Get("/api/friends");
        if (res) {
            HealIfRejected(res->status);
            json response = json::parse(res->body);

            if (response.contains("friends") && response["friends"].is_array()) {
                for (const auto& f : response["friends"]) {
                    friends.push_back(ParseFriend(f.dump()));
                }
            }
            if (response.contains("requests") && response["requests"].is_array()) {
                for (const auto& f : response["requests"]) {
                    requests.push_back(ParseFriend(f.dump()));
                }
            }
        }
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] GetSocial failed: {}", ex.what());
    }
    return {friends, requests};
}

std::tuple<bool, std::string> NextendoApi::AddFriend(const std::string& friendCode) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        json payload = {{"friend_code", friendCode}};
        auto res = client.Post("/api/friends", payload.dump(), "application/json");

        if (!res) {
            return {false, "Failed to connect"};
        }

        json response = json::parse(res->body);
        if (res->status == 200 && response.contains("friend")) {
            Friend f = ParseFriend(response["friend"].dump());
            bool already = response.value("already", false);
            return {true, already ? "Already friends" : "Friend request sent to " + f.name};
        }

        return {false, response.value("error", "Error")};
    } catch (const std::exception& ex) {
        return {false, ex.what()};
    }
}

bool NextendoApi::AcceptFriend(uint64_t pid) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        json payload = {{"pid", pid}};
        auto res = client.Post("/api/friends/accept", payload.dump(), "application/json");
        return res && res->status == 200;
    } catch (...) {
        return false;
    }
}

int NextendoApi::AcceptAllRequests() {
    auto social = GetSocial();
    auto& requests = std::get<1>(social);
    int accepted = 0;
    for (const auto& r : requests) {
        if (r.pid != 0 && AcceptFriend(r.pid)) {
            accepted++;
        }
    }
    return accepted;
}

void NextendoApi::DeclineFriend(uint64_t pid) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        json payload = {{"pid", pid}};
        client.Post("/api/friends/decline", payload.dump(), "application/json");
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] Decline failed: {}", ex.what());
    }
}

void NextendoApi::RemoveFriend(uint64_t pid) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        json payload = {{"pid", pid}};
        client.Post("/api/friends/remove", payload.dump(), "application/json");
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] RemoveFriend failed: {}", ex.what());
    }
}

void NextendoApi::SetFavorite(uint64_t pid, bool favorite) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        json payload = {{"pid", pid}, {"favorite", favorite}};
        client.Post("/api/friends/favorite", payload.dump(), "application/json");
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] SetFavorite failed: {}", ex.what());
    }
}

std::tuple<bool, std::string> NextendoApi::SignInWithBrowser() {
    // OAuth 2.0 with PKCE implementation
    try {
        // PKCE (S256) + CSRF state
        std::string verifier = CryptoUtils::GenerateRandomUrlString(32);
        std::vector<uint8_t> verifier_bytes(verifier.begin(), verifier.end());
        std::vector<uint8_t> hash = CryptoUtils::SHA256(verifier_bytes);
        std::string challenge = CryptoUtils::Base64UrlEncode(hash);
        std::string state = CryptoUtils::GenerateRandomUrlString(24);

        // Start OAuth listener
        OAuthListener listener;
        int port = listener.Start();
        if (port == 0) {
            return {false, "Failed to start OAuth listener"};
        }

        std::string redirectUri = "http://127.0.0.1:" + std::to_string(port) + "/callback";

        // Build authorization URL
        std::string baseUrl = GetBaseUrl();
        std::string authorizeUrl = baseUrl + "/api/oauth/authorize?response_type=code"
                                   "&client_id=nextendo-emulator"
                                   "&redirect_uri=" + redirectUri +
                                   "&scope=identity+friends"
                                   "&state=" + state +
                                   "&code_challenge=" + challenge +
                                   "&code_challenge_method=S256";

        // Open browser
        if (!PlatformUtils::OpenUrl(authorizeUrl)) {
            listener.Stop();
            return {false, "Failed to open browser"};
        }

        // Get the callback data (set callback BEFORE waiting)
        std::string code, callback_state, error;
        listener.SetCallback([&](const std::string& c, const std::string& s, const std::string& e) {
            code = c;
            callback_state = s;
            error = e;
        });

        // Wait for callback (5 minute timeout)
        if (!listener.WaitForCallback(300)) {
            listener.Stop();
            return {false, "Connection timeout"};
        }

        listener.Stop();

        if (!error.empty()) {
            return {false, error == "access_denied" ? "Connection refused" : error};
        }

        if (code.empty()) {
            return {false, "No code received from browser"};
        }

        if (callback_state != state) {
            return {false, "CSRF verification failed"};
        }

        // Exchange code for token
        httplib::Client client(baseUrl);
        client.set_connection_timeout(15);
        client.set_read_timeout(15);
        client.set_write_timeout(15);

        std::string form_data = "grant_type=authorization_code"
                               "&code=" + code +
                               "&client_id=nextendo-emulator"
                               "&redirect_uri=" + redirectUri +
                               "&code_verifier=" + verifier;

        auto res = client.Post("/api/oauth/token", form_data, "application/x-www-form-urlencoded");

        if (!res || res->status != 200) {
            return {false, "Failed to exchange authorization code"};
        }

        json response = json::parse(res->body);
        std::string nexToken = response.value("nex_token", "");

        if (nexToken.empty() || !response.contains("account")) {
            return {false, "Invalid authentication response"};
        }

        uint64_t pid = response["account"].value("pid", 0);
        std::string username = response["account"].value("username", "");
        std::string friendCode = response["account"].value("friend_code", "");

        if (pid == 0 || nexToken.empty()) {
            return {false, "Invalid account data in response"};
        }

        NextendoAccount::Save(pid, username, friendCode, nexToken, false);
        return {true, ""};

    } catch (const std::exception& ex) {
        return {false, ex.what()};
    }
}

std::tuple<bool, std::string> NextendoApi::SetUsername(const std::string& username) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        json payload = {{"username", username}};
        auto res = client.Put("/api/username", payload.dump(), "application/json");

        if (res && res->status == 200) {
            return {true, ""};
        }

        json response = json::parse(res->body);
        return {false, response.value("error", "Error")};
    } catch (const std::exception& ex) {
        return {false, ex.what()};
    }
}

bool NextendoApi::SetProfileImage(const std::vector<uint8_t>& jpeg) {
    try {
        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        // Get current profile to preserve name and mii
        std::string name = NextendoAccount::GetUsername();
        std::string mii = NextendoAccount::GetMiiData();

        // Encode image to base64 (simplified)
        std::string image_base64; // TODO: proper base64 encoding

        json payload = {
            {"name", name},
            {"image", image_base64},
            {"mii", mii}
        };

        auto res = client.Put("/api/profile", payload.dump(), "application/json");
        return res && res->status == 200;
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] SetProfileImage failed: {}", ex.what());
        return false;
    }
}

std::tuple<bool, std::string> NextendoApi::SendReport(const std::string& errorCode,
                                                     const std::string& comment,
                                                     bool attachLog) {
    if (NextendoAccount::GetNexToken().empty()) {
        return {false, "Connect to Nextendo to report a problem."};
    }

    try {
        json payload = {
            {"error_code", errorCode},
            {"game", ""}, // TODO: get current title ID
            {"version", ""}, // TODO: get version
            {"comment", comment},
            {"log", attachLog ? ReadLogTail() : ""}
        };

        httplib::Client client(GetBaseUrl());
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        auto res = client.Post("/api/report", payload.dump(), "application/json");

        if (res && res->status == 200) {
            return {true, ""};
        }

        json response = json::parse(res->body);
        return {false, response.value("error", "Failed to send report")};
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] Report failed: {}", ex.what());
        return {false, "Unable to send. Check your connection."};
    }
}

std::string NextendoApi::ReadLogTail() {
    // TODO: Read the last 96KB of the log file
    // This requires knowing the log directory path
    return "";
}

NextendoApi::Friend NextendoApi::ParseFriend(const std::string& json_str) {
    NextendoApi::Friend f;
    try {
        auto j = nlohmann::json::parse(json_str);
        f.pid = j.value("pid", (uint64_t)0);
        f.username = j.value("username", "");
        f.name = j.value("name", "");
        f.friend_code = j.value("friend_code", "");
        f.image_base64 = j.value("image", "");
        f.favorite = j.value("favorite", false);

        if (j.contains("presence") && j["presence"].is_object()) {
            auto& presence = j["presence"];
            f.online_status = presence.value("status", 0);
            f.app_id = presence.value("app_id", "");
            f.app_detail = presence.value("app_detail", "");
        }
    } catch (...) {
        // Return empty friend on parse error
    }
    return f;
}

} // namespace Common::Nextendo

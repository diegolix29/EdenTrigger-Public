// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace Common::Nextendo {

/// Thin HTTP client for the Nextendo account service, used by the
/// Settings → Nextendo page (profile + friends). Authenticates with the locally
/// persisted NEX token. All calls are best-effort; errors are returned as text.
class NextendoApi {
public:
    // Release channel this build belongs to
    static constexpr char ReleaseChannel[] = "v1";

    struct Friend {
        uint64_t pid = 0;
        std::string username;
        std::string name;
        std::string friend_code;
        std::string image_base64;
        bool favorite = false;
        int online_status = 0;
        std::string app_id;
        std::string app_detail;

        bool IsOnline() const { return online_status != 0; }
    };

    struct HistoryItem {
        std::string title_id;
        std::string name;
        std::string icon_base64;
        int64_t seconds = 0;
        std::string last_played;
    };

    struct BetaConfig {
        bool online_enabled = false;
        std::string min_app_version = "0.0.0";
        std::string message_en;
        std::string message_fr;
        std::string force_update_url;
    };

    // Event raised when the server rejects our stored token (HTTP 401)
    static std::function<void()> SessionRevoked;

    // API Methods
    static std::vector<HistoryItem> SyncHistory(const std::vector<HistoryItem>& local);
    static std::string GetBaseUrl();
    static std::string GetSiteUrl();

    static std::string GetOnlineRefusalReason();
    static std::tuple<std::string, std::vector<uint8_t>> GetProfileSync();

    static std::tuple<bool, std::string> CreateGuest(const std::string& nickname);
    static std::optional<bool> CheckNicknameAvailable(const std::string& nickname);

    static std::tuple<BetaConfig, bool> GetBetaConfig();
    static void TouchSession();

    static std::tuple<std::vector<Friend>, std::vector<Friend>> GetSocial();
    static std::tuple<bool, std::string> AddFriend(const std::string& friendCode);
    static bool AcceptFriend(uint64_t pid);
    static int AcceptAllRequests();
    static void DeclineFriend(uint64_t pid);
    static void RemoveFriend(uint64_t pid);
    static void SetFavorite(uint64_t pid, bool favorite);

    static std::tuple<bool, std::string> SignInWithBrowser();
    static std::tuple<bool, std::string> SetUsername(const std::string& username);
    static bool SetProfileImage(const std::vector<uint8_t>& jpeg);
    static std::tuple<bool, std::string> SendReport(const std::string& errorCode,
                                                   const std::string& comment,
                                                   bool attachLog = true);

private:
    static bool HealIfRejected(int statusCode);
    static std::string ReadLogTail();
    static Friend ParseFriend(const std::string& json);
};

} // namespace Common::Nextendo

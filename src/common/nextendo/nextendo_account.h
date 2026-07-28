// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <mutex>

namespace Common::Nextendo {

/// Nextendo Network linked account. Written by the in-app "Connexion Nextendo
/// Network" dialog, read by the Account service (ManagerServer) so the NEX
/// login presents the account's PERSISTENT principal id (PID) instead of the
/// 0xcafe stub — this is what makes "log in with your account = play online as
/// you" work. Stored as a tiny key=value file (no JSON, trimming/AOT-safe).
class NextendoAccount {
public:
    static uint64_t GetPid();
    static std::string GetUsername();
    static std::string GetFriendCode();
    static std::string GetNexToken();

    /// True when the linked identity is a no-account GUEST profile (created by
    /// the beta quick-start via /api/guest) rather than a full registered account.
    static bool IsGuest();

    /// The Ryujinx user-profile UserId bound to this Nextendo account
    static std::string GetProfileUserId();

    /// Base64 of the account's Mii (Switch StoreData, 0x44 bytes), mirrored locally
    static std::string GetMiiData();

    static bool IsLinked();

    /// Runtime-only (not persisted): set true at game launch when the running
    /// game is a Nextendo title on an UNSUPPORTED version.
    static bool IsOnlineBlocked();
    static void SetOnlineBlocked(bool blocked);

    static void Save(uint64_t pid, const std::string& username, const std::string& friendCode,
                    const std::string& nexToken, bool isGuest = false);
    static void SetProfileUserId(const std::string& userId);
    static void SetMiiData(const std::string& miiData);
    static void Clear();

private:
    static void EnsureLoaded();
    static void WriteFileLocked();

    static std::string GetFilePath();

    static std::mutex s_lock;
    static bool s_loaded;
    static uint64_t s_pid;
    static std::string s_username;
    static std::string s_friendCode;
    static std::string s_nexToken;
    static bool s_isGuest;
    static std::string s_profileUserId;
    static std::string s_miiData;
    static bool s_onlineBlocked;
};

} // namespace Common::Nextendo

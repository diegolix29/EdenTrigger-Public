// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_account.h"
#include "common/fs/fs.h"
#include "common/fs/fs_util.h"
#include "common/fs/path_util.h"
#include "common/fs/fs_paths.h"
#include "common/logging.h"
#include <fstream>
#include <sstream>

namespace Common::Nextendo {

// Static member initialization
std::mutex NextendoAccount::s_lock;
bool NextendoAccount::s_loaded = false;
uint64_t NextendoAccount::s_pid = 0;
std::string NextendoAccount::s_username;
std::string NextendoAccount::s_friendCode;
std::string NextendoAccount::s_nexToken;
bool NextendoAccount::s_isGuest = false;
std::string NextendoAccount::s_profileUserId;
std::string NextendoAccount::s_miiData;
bool NextendoAccount::s_onlineBlocked = false;

uint64_t NextendoAccount::GetPid() {
    EnsureLoaded();
    return s_pid;
}

std::string NextendoAccount::GetUsername() {
    EnsureLoaded();
    return s_username;
}

std::string NextendoAccount::GetFriendCode() {
    EnsureLoaded();
    return s_friendCode;
}

std::string NextendoAccount::GetNexToken() {
    EnsureLoaded();
    return s_nexToken;
}

bool NextendoAccount::IsGuest() {
    EnsureLoaded();
    return s_isGuest;
}

std::string NextendoAccount::GetProfileUserId() {
    EnsureLoaded();
    return s_profileUserId;
}

std::string NextendoAccount::GetMiiData() {
    EnsureLoaded();
    return s_miiData;
}

bool NextendoAccount::IsLinked() {
    EnsureLoaded();
    return s_pid != 0;
}

bool NextendoAccount::IsOnlineBlocked() {
    return s_onlineBlocked;
}

void NextendoAccount::SetOnlineBlocked(bool blocked) {
    s_onlineBlocked = blocked;
}

void NextendoAccount::Save(uint64_t pid, const std::string& username, const std::string& friendCode,
                          const std::string& nexToken, bool isGuest) {
    std::lock_guard<std::mutex> lock(s_lock);

    EnsureLoaded(); // preserve an existing profile binding for the same account
    if (pid != s_pid) {
        // different account -> unbind the previous local profile + Mii
        s_profileUserId.clear();
        s_miiData.clear();
    }

    s_pid = pid;
    s_username = username;
    s_friendCode = friendCode;
    s_nexToken = nexToken;
    s_isGuest = isGuest;
    s_loaded = true;
    WriteFileLocked();
}

void NextendoAccount::SetProfileUserId(const std::string& userId) {
    std::lock_guard<std::mutex> lock(s_lock);
    EnsureLoaded();
    s_profileUserId = userId;
    WriteFileLocked();
}

void NextendoAccount::SetMiiData(const std::string& miiData) {
    std::lock_guard<std::mutex> lock(s_lock);
    EnsureLoaded();
    s_miiData = miiData;
    WriteFileLocked();
}

void NextendoAccount::Clear() {
    std::lock_guard<std::mutex> lock(s_lock);
    s_pid = 0;
    s_username.clear();
    s_friendCode.clear();
    s_nexToken.clear();
    s_profileUserId.clear();
    s_miiData.clear();
    s_isGuest = false;
    s_loaded = true;
    s_onlineBlocked = false;

    try {
        const auto path = GetFilePath();
        if (FS::Exists(path)) {
            FS::RemoveFile(path);
        }
    } catch (...) {
        // ignore
    }
}

void NextendoAccount::EnsureLoaded() {
    std::lock_guard<std::mutex> lock(s_lock);

    if (s_loaded) {
        return;
    }

    s_loaded = true;

    try {
        const auto path = GetFilePath();
        if (!FS::Exists(path)) {
            return;
        }

        std::ifstream file(path, std::ios::in | std::ios::binary);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        if (content.empty()) {
            return;
        }

        // Parse key=value lines
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            size_t eq = line.find('=');
            if (eq == std::string::npos || eq == 0) {
                continue;
            }

            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);

            if (key == "pid") {
                s_pid = std::stoull(val);
            } else if (key == "username") {
                s_username = val;
            } else if (key == "friend_code") {
                s_friendCode = val;
            } else if (key == "nex_token") {
                s_nexToken = val;
            } else if (key == "profile_user_id") {
                s_profileUserId = val;
            } else if (key == "mii_data") {
                s_miiData = val;
            } else if (key == "is_guest") {
                s_isGuest = (val == "1" || val == "true");
            }
        }
    } catch (...) {
        // Corrupt/unreadable file -> treat as not linked.
        s_pid = 0;
    }
}

void NextendoAccount::WriteFileLocked() {
    try {
        const auto path = GetFilePath();
        std::string content = fmt::format("pid={}\nusername={}\nfriend_code={}\nnex_token={}\n"
                                           "profile_user_id={}\nmii_data={}\nis_guest={}\n",
                                           s_pid, s_username, s_friendCode, s_nexToken,
                                           s_profileUserId, s_miiData, s_isGuest ? "1" : "0");
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        file << content;
        file.close();
    } catch (...) {
        // Best effort; the in-memory values still apply this session.
    }
}

std::string NextendoAccount::GetFilePath() {
    // TODO: Use the proper base directory path from Eden's configuration
    // For now, use a temporary location
    return std::filesystem::path(Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nextendo_account.txt").string();
}

} // namespace Common::Nextendo

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_beta.h"
#include "common/nextendo/nextendo_api.h"
#include "common/logging.h"
#include <thread>
#include <chrono>

namespace Common::Nextendo {

// Static callback initialization
std::function<std::string()> NextendoBeta::GetAppVersion = nullptr;
std::function<bool()> NextendoBeta::IsReleaseBuild = nullptr;
std::function<std::string(const std::string& key)> NextendoBeta::GetLocalizedString = nullptr;

// Static state
static std::mutex s_lock;
static NextendoApi::BetaConfig s_cfg;
static bool s_reachable = false;
static bool s_polled = false;

bool NextendoBeta::HasPolled() {
    std::lock_guard<std::mutex> lock(s_lock);
    return s_polled;
}

std::string NextendoBeta::GetForceUpdateUrl() {
    std::lock_guard<std::mutex> lock(s_lock);
    return s_cfg.force_update_url;
}

void NextendoBeta::Refresh() {
    auto [cfg, reachable] = NextendoApi::GetBetaConfig();
    std::lock_guard<std::mutex> lock(s_lock);
    s_cfg = cfg;
    s_reachable = reachable;
    s_polled = true;
}

NextendoBeta::BlockReason NextendoBeta::Evaluate() {
    // [Nextendo] DEV / unstamped local builds bypass EVERY gate: no
    // forced-update popup, no kill-switch, no version block. Local testing stays
    // unrestricted while stamped release builds keep all their safeguards.
    if (IsReleaseBuild && !IsReleaseBuild()) {
        return BlockReason::None;
    }

    std::lock_guard<std::mutex> lock(s_lock);

    if (!s_polled || !s_reachable) {
        return BlockReason::Unreachable;
    }

    if (!s_cfg.online_enabled) {
        return BlockReason::Disabled;
    }

    if (IsClientOutdated(s_cfg.min_app_version)) {
        return BlockReason::UpdateRequired;
    }

    return BlockReason::None;
}

std::string NextendoBeta::GetMessage(BlockReason reason) {
    // For now, return English defaults. Localized messages can be added via callback
    switch (reason) {
    case BlockReason::Disabled:
        // Prefer the operator's own message (set on the server) if present.
        if (!s_cfg.message_en.empty()) {
            return s_cfg.message_en;
        }
        if (GetLocalizedString) {
            return GetLocalizedString("Dialog_Nextendo_OnlineDisabledByOperator");
        }
        return "Online services have been disabled by the operator.";

    case BlockReason::UpdateRequired:
        if (GetLocalizedString) {
            return GetLocalizedString("Dialog_Nextendo_OnlineUpdateRequired");
        }
        return "An update is required to use online services.";

    case BlockReason::Unreachable:
    default:
        if (GetLocalizedString) {
            return GetLocalizedString("Dialog_Nextendo_OnlineServersUnreachable");
        }
        return "Online servers are currently unreachable.";
    }
}

bool NextendoBeta::IsClientOutdated(const std::string& min_version) {
    int min_major = 0, min_minor = 0, min_patch = 0, min_revision = 0;
    if (!TryParseVersion(min_version, min_major, min_minor, min_patch, min_revision)) {
        return false; // no/invalid minimum -> don't gate on version
    }

    if (!GetAppVersion) {
        return false; // can't read our own version -> don't hard-block on it
    }

    std::string cur_version = GetAppVersion();
    int cur_major = 0, cur_minor = 0, cur_patch = 0, cur_revision = 0;
    if (!TryParseVersion(cur_version, cur_major, cur_minor, cur_patch, cur_revision)) {
        return false; // can't parse our own version -> don't hard-block on it
    }

    // Compare versions
    if (cur_major < min_major) return true;
    if (cur_major > min_major) return false;

    if (cur_minor < min_minor) return true;
    if (cur_minor > min_minor) return false;

    if (cur_patch < min_patch) return true;
    if (cur_patch > min_patch) return false;

    if (cur_revision < min_revision) return true;

    return false;
}

bool NextendoBeta::TryParseVersion(const std::string& s, int& major, int& minor, int& patch, int& revision) {
    major = 0;
    minor = 0;
    patch = 0;
    revision = 0;

    if (s.empty()) {
        return false;
    }

    // Parse leading "major.minor[.build[.rev]]" and ignore any suffix
    size_t i = 0;
    while (i < s.length() && (std::isdigit(s[i]) || s[i] == '.')) {
        i++;
    }

    std::string core = s.substr(0, i);

    // Parse components
    size_t pos = 0;
    size_t dot_pos = core.find('.', pos);

    if (dot_pos == std::string::npos) {
        return false;
    }

    major = std::stoi(core.substr(pos, dot_pos - pos));
    pos = dot_pos + 1;

    dot_pos = core.find('.', pos);
    if (dot_pos != std::string::npos) {
        minor = std::stoi(core.substr(pos, dot_pos - pos));
        pos = dot_pos + 1;

        dot_pos = core.find('.', pos);
        if (dot_pos != std::string::npos) {
            patch = std::stoi(core.substr(pos, dot_pos - pos));
            pos = dot_pos + 1;
            revision = std::stoi(core.substr(pos));
        } else {
            patch = std::stoi(core.substr(pos));
        }
    } else {
        minor = std::stoi(core.substr(pos));
    }

    return true;
}

} // namespace Common::Nextendo

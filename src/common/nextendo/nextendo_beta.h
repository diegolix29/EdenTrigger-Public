// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <mutex>
#include <functional>

namespace Common::Nextendo {

/// [Nextendo] Remote beta gate — the kill-switch + forced-update control. Polls
/// /api/beta-config and FAILS CLOSED: until a successful response says otherwise,
/// online play is blocked. The operator disables online for everyone (online_enabled=false)
/// or forces a minimum client version (min_app_version) by editing the server config alone;
/// no client update is needed to flip the switch.
class NextendoBeta {
public:
    enum class BlockReason {
        None,
        Unreachable,
        Disabled,
        UpdateRequired,
    };

    // Callback to get the current application version
    static std::function<std::string()> GetAppVersion;
    // Callback to check if this is a valid release build (vs dev build)
    static std::function<bool()> IsReleaseBuild;
    // Callback for localized messages
    static std::function<std::string(const std::string& key)> GetLocalizedString;

    /// True once at least one poll (success or failure) has completed. The launch gate
    /// awaits an initial poll before evaluating so the first online launch isn't falsely blocked.
    static bool HasPolled();

    /// Download page for the mandatory update, as set by the server. Empty if none.
    static std::string GetForceUpdateUrl();

    /// Fetches the latest remote config. Safe to call repeatedly (startup + each launch).
    static void Refresh();

    /// Whether online is currently blocked, and why. FAIL CLOSED: blocked until a
    /// successful poll confirms online is enabled and the client is up to date.
    static BlockReason Evaluate();

    /// Localized, user-facing reason online is unavailable.
    static std::string GetMessage(BlockReason reason);

private:
    static bool IsClientOutdated(const std::string& min_version);
    static bool TryParseVersion(const std::string& s, int& major, int& minor, int& patch, int& revision);
};

} // namespace Common::Nextendo

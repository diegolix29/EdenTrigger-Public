// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace Common::Nextendo {

/// Platform-specific utilities for Nextendo integration
class PlatformUtils {
public:
    /// Open a URL in the default browser
    /// Returns true on success, false on failure
    static bool OpenUrl(const std::string& url);
};

} // namespace Common::Nextendo

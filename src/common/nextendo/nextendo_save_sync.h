// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace Common::Nextendo {

/// [Nextendo] Game-save cloud sync. On launch of a Nextendo-compatible title (only),
/// the account's save for that title is downloaded and applied locally BEFORE the
/// game runs; when the game closes, the local save is uploaded back to the account.
/// So progression follows the Nextendo account across machines. Best-effort: any
/// failure is logged and ignored (the game still runs with whatever is local).
class NextendoSaveSync {
public:
    // Callbacks for Eden-specific save data operations
    // These must be set before using Pull/Push functionality
    static std::function<bool()> IsNextendoProfileActive;
    static std::function<bool(uint64_t title_id)> LocalSaveHasContent;
    static std::function<bool(uint64_t title_id, const std::vector<uint8_t>& zip_data)> ImportNextendoSave;
    static std::function<std::vector<uint8_t>(uint64_t title_id)> ExportNextendoSave;

    // Download + import this title's cloud save. Call BEFORE the game starts.
    static void Pull(uint64_t title_id, const std::string& title_id_string);

    // Export + upload this title's local save. Call AFTER the game has fully closed.
    static void Push(uint64_t title_id, const std::string& title_id_string);

private:
    static std::string GetBaseUrl();
};

} // namespace Common::Nextendo

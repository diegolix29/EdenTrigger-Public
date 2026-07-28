// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>
#include <functional>

namespace Common::Nextendo {

/// [Nextendo] Public bridge that lets the UI sync a Nextendo account's Mii into the
/// local Switch Mii database (system save 0x8000000000000030) WITHOUT a running game.
/// A Mii is persisted on the account as raw StoreData bytes (0x44); the
/// fixed Eden device id makes those bytes valid on every Eden install, so the Mii
/// follows the account across machines.
/// All operations are best-effort and never throw to the caller.
class NextendoMiiSync {
public:
    // Callbacks for Eden-specific Mii database operations
    // These must be set before using Inject/Remove functionality
    static std::function<bool(const std::vector<uint8_t>& store_data_bytes)> InjectMii;
    static std::function<bool(const std::vector<uint8_t>& store_data_bytes)> RemoveMii;

    /// Build a fresh Mii and return its StoreData bytes (length 0x44), or empty vector on failure.
    static std::vector<uint8_t> CreateMii();

    /// Add (or replace) the given Mii in the local database. Returns true on success.
    static bool Inject(const std::vector<uint8_t>& store_data_bytes);

    /// Remove the given Mii (matched by its CreateId) from the local database.
    static bool Remove(const std::vector<uint8_t>& store_data_bytes);
};

} // namespace Common::Nextendo

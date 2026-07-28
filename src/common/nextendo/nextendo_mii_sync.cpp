// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_mii_sync.h"
#include "common/logging.h"
#include <random>
#include <cstring>

namespace Common::Nextendo {

// Static callback initialization
std::function<bool(const std::vector<uint8_t>& store_data_bytes)> NextendoMiiSync::InjectMii = nullptr;
std::function<bool(const std::vector<uint8_t>& store_data_bytes)> NextendoMiiSync::RemoveMii = nullptr;

std::vector<uint8_t> NextendoMiiSync::CreateMii() {
    try {
        // For now, return empty since we need access to Eden's StoreData class
        // This will need to be implemented with proper Mii creation
        // TODO: Integrate with Eden's StoreData::BuildDefault or similar
        LOG_WARNING(Common, "[Nextendo] CreateMii not fully implemented - needs Eden StoreData integration");
        return {};
    } catch (...) {
        return {};
    }
}

bool NextendoMiiSync::Inject(const std::vector<uint8_t>& store_data_bytes) {
    if (store_data_bytes.empty() || store_data_bytes.size() != 0x44) {
        LOG_WARNING(Common, "[Nextendo] InjectMii: invalid store data size (expected 0x44, got {})",
                    store_data_bytes.size());
        return false;
    }

    if (!InjectMii) {
        LOG_WARNING(Common, "[Nextendo] InjectMii: callback not set");
        return false;
    }

    try {
        return InjectMii(store_data_bytes);
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] InjectMii failed: {}", ex.what());
        return false;
    }
}

bool NextendoMiiSync::Remove(const std::vector<uint8_t>& store_data_bytes) {
    if (store_data_bytes.empty() || store_data_bytes.size() != 0x44) {
        LOG_WARNING(Common, "[Nextendo] RemoveMii: invalid store data size (expected 0x44, got {})",
                    store_data_bytes.size());
        return false;
    }

    if (!RemoveMii) {
        LOG_WARNING(Common, "[Nextendo] RemoveMii: callback not set");
        return false;
    }

    try {
        return RemoveMii(store_data_bytes);
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] RemoveMii failed: {}", ex.what());
        return false;
    }
}

} // namespace Common::Nextendo

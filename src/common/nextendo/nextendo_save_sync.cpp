// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_save_sync.h"
#include "common/nextendo/nextendo_account.h"
#include "common/nextendo/nextendo_endpoint.h"
#include "common/logging.h"
#include "common/httplib.h"
#include <thread>
#include <chrono>

namespace Common::Nextendo {

// Static callback initialization
std::function<bool()> NextendoSaveSync::IsNextendoProfileActive = nullptr;
std::function<bool(uint64_t title_id)> NextendoSaveSync::LocalSaveHasContent = nullptr;
std::function<bool(uint64_t title_id, const std::vector<uint8_t>& zip_data)> NextendoSaveSync::ImportNextendoSave = nullptr;
std::function<std::vector<uint8_t>(uint64_t title_id)> NextendoSaveSync::ExportNextendoSave = nullptr;

std::string NextendoSaveSync::GetBaseUrl() {
    return NextendoEndpoint::GetBaseUrl();
}

void NextendoSaveSync::Pull(uint64_t title_id, const std::string& title_id_string) {
    // [Nextendo beta] Cloud saves are excluded for GUEST profiles — skip pull entirely.
    if (!NextendoAccount::IsLinked() || NextendoAccount::IsGuest() || NextendoAccount::GetNexToken().empty()) {
        return;
    }

    // Only sync on the profile bound to the Nextendo account (online profile). A local/other
    // profile must NOT pull the account's cloud save onto its own (empty) save.
    if (IsNextendoProfileActive && !IsNextendoProfileActive()) {
        LOG_INFO(Common, "[Nextendo] save pull {}: skipped (active profile is not the Nextendo one)", title_id_string);
        return;
    }

    // Never overwrite real local progress: the cloud copy is only restored onto a
    // machine that has NO local save yet (fresh install / new PC). With a local save
    // present, IT is the player's progress — pulling a possibly-older cloud copy every
    // launch would reset things (VR, last character, …). The push keeps the cloud backed up.
    if (LocalSaveHasContent && LocalSaveHasContent(title_id)) {
        LOG_INFO(Common, "[Nextendo] save pull {}: local save present -> kept (no overwrite)", title_id_string);
        return;
    }

    try {
        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(30);
        client.set_read_timeout(30);
        client.set_write_timeout(30);
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        std::string url = "/api/save/" + title_id_string;
        auto res = client.Get(url);

        if (!res || res->status == 204 || !res->status >= 200 || res->status >= 300) {
            return; // no cloud save stored yet
        }

        std::vector<uint8_t> zip_data(res->body.begin(), res->body.end());
        if (!zip_data.empty()) {
            if (ImportNextendoSave) {
                bool ok = ImportNextendoSave(title_id, zip_data);
                LOG_INFO(Common, "[Nextendo] save pull {}: {} ({} B)", title_id_string, ok ? "applied" : "skipped", zip_data.size());
            }
        }
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] save pull failed: {}", ex.what());
    }
}

void NextendoSaveSync::Push(uint64_t title_id, const std::string& title_id_string) {
    LOG_INFO(Common, "[Nextendo] save push START {} (linked={} token={})", title_id_string,
             NextendoAccount::IsLinked(), !NextendoAccount::GetNexToken().empty());

    // [Nextendo beta] Cloud saves are excluded for GUEST profiles — never push.
    if (!NextendoAccount::IsLinked() || NextendoAccount::IsGuest() || NextendoAccount::GetNexToken().empty()) {
        return;
    }

    // Only the Nextendo-bound profile uploads its save to the account. A local/other profile
    // pushing its (empty) save would clobber the account's cloud backup.
    if (IsNextendoProfileActive && !IsNextendoProfileActive()) {
        LOG_INFO(Common, "[Nextendo] save push {}: skipped (active profile is not the Nextendo one)", title_id_string);
        return;
    }

    try {
        if (!ExportNextendoSave) {
            LOG_INFO(Common, "[Nextendo] save push {}: export callback not set -> nothing to upload", title_id_string);
            return;
        }

        std::vector<uint8_t> zip_data = ExportNextendoSave(title_id);
        if (zip_data.empty()) {
            LOG_INFO(Common, "[Nextendo] save push {}: export empty -> nothing to upload", title_id_string);
            return;
        }

        httplib::Client client(GetBaseUrl());
        client.set_connection_timeout(60);
        client.set_read_timeout(60);
        client.set_write_timeout(60);
        client.set_bearer_token_auth(NextendoAccount::GetNexToken());

        std::string url = "/api/save/" + title_id_string;
        auto res = client.Put(url, reinterpret_cast<const char*>(zip_data.data()), zip_data.size(), "application/octet-stream");

        LOG_INFO(Common, "[Nextendo] save push {}: HTTP {} ({} B)", title_id_string, res ? res->status : 0, zip_data.size());
    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] save push failed: {}", ex.what());
    }
}

} // namespace Common::Nextendo

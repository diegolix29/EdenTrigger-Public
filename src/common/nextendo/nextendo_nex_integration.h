// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include "common/nextendo/nextendo_account.h"

namespace Common::Nextendo {

/// [Nextendo] Integration point for Eden's NEX (Nintendo Network) service.
/// When Eden's NEX service implementation needs the Nextendo account PID for
/// online login, it should call this function to retrieve the linked account's
/// Player ID. Returns 0 if no Nextendo account is linked.
///
/// This allows Nextendo accounts to be used for online play in games that
/// support Nintendo Network services.
inline uint64_t GetNexPid() {
    return NextendoAccount::GetPid();
}

/// Check if a Nextendo account is linked and ready for online login.
/// This should be called before attempting to use NEX services.
inline bool IsNexReady() {
    return NextendoAccount::IsLinked() && !NextendoAccount::IsOnlineBlocked();
}

/// Get the NEX token for authentication with Nintendo Network services.
/// This token is stored in the Nextendo account after successful sign-in.
inline std::string GetNexToken() {
    return NextendoAccount::GetNexToken();
}

} // namespace Common::Nextendo

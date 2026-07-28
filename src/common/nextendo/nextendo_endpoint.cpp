// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_endpoint.h"
#include "common/logging.h"
#include <cstdlib>
#include <algorithm>

namespace Common::Nextendo {

// Static member initialization
std::string NextendoEndpoint::s_cached;
bool NextendoEndpoint::s_resolved = false;

std::string NextendoEndpoint::GetBaseUrl() {
    if (s_resolved) {
        return s_cached;
    }

    s_cached = Resolve();
    s_resolved = true;

    return s_cached;
}

std::string NextendoEndpoint::Resolve() {
    const char* env_raw = std::getenv(EnvVar);
    std::string raw = env_raw ? env_raw : "";

    // Smart-quote pastes (from Discord, typically) glue a curly quote to the host, which
    // IDNA then turns into "nextendo.xn--network-b46c" and DNS fails.
    // Trim whitespace and quotes
    const std::string quotes = "\"'""''""''"; // Regular quotes and smart quotes
    raw.erase(0, raw.find_first_not_of(" \t\r\n"));
    raw.erase(raw.find_last_not_of(" \t\r\n") + 1);
    while (!raw.empty() && quotes.find(raw.back()) != std::string::npos) {
        raw.pop_back();
    }
    while (!raw.empty() && quotes.find(raw.front()) != std::string::npos) {
        raw.erase(0, 1);
    }

    if (raw.empty()) {
        return Canonical;
    }

    // Check if it's a valid URL
    // Simple check - must start with http:// or https://
    if (raw.find("http://") != 0 && raw.find("https://") != 0) {
        Reject(raw, "not a valid absolute url");
        return Canonical;
    }

    // Parse the URL to extract scheme and host
    size_t scheme_end = raw.find("://");
    std::string scheme = raw.substr(0, scheme_end);
    std::string rest = raw.substr(scheme_end + 3);

    size_t host_end = rest.find('/');
    std::string host = (host_end != std::string::npos) ? rest.substr(0, host_end) : rest;

    // Loopback: a developer pointing at their own machine. Any scheme, any port — nothing
    // leaves the machine, so there is nothing to protect against here.
    if (host == "localhost" || host == "127.0.0.1" || host == "::1" || host == "[::1]") {
        // Remove trailing slash
        if (!raw.empty() && raw.back() == '/') {
            raw.pop_back();
        }
        return raw;
    }

    // Anything else must be Nextendo, over TLS. http:// would put the token on the wire in
    // cleartext even against the real server.
    if (scheme != "https") {
        Reject(raw, "only https is accepted for a remote server");
        return Canonical;
    }

    if (!IsNextendoHost(host)) {
        Reject(raw, "host is not a nextendo.network address");
        return Canonical;
    }

    // Remove trailing slash
    if (!raw.empty() && raw.back() == '/') {
        raw.pop_back();
    }
    return raw;
}

bool NextendoEndpoint::IsNextendoHost(const std::string& host) {
    constexpr char Domain[] = "nextendo.network";

    // Exact match
    if (host == Domain) {
        return true;
    }

    // Subdomain check (must have a dot before the domain)
    std::string with_dot = "." + std::string(Domain);
    if (host.length() > with_dot.length() &&
        host.substr(host.length() - with_dot.length()) == with_dot) {
        return true;
    }

    return false;
}

void NextendoEndpoint::Reject(const std::string& value, const std::string& why) {
    // Loud on purpose: someone who set this deserves to know it was ignored, and someone
    // who did not set it deserves to see that something tried.
    LOG_WARNING(Common, "[Nextendo] Ignoring {}=\"{}\" ({}). Using {}. "
               "Your account token is only ever sent to Nextendo.",
               EnvVar, value, why, Canonical);
}

} // namespace Common::Nextendo

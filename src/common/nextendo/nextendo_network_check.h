// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

namespace Common::Nextendo {

/// Connection checks the player can run on demand, without launching a game: how
/// far away the servers are, and whether their NAT lets other players reach them directly.
/// Both come out of a single probe of the NAT-check servers (nncs), because that is the one
/// piece of infrastructure that speaks UDP to the same path the games use for P2P.
class NextendoNetworkCheck {
public:
    enum class NatType {
        /// Probe failed — servers unreachable, or UDP blocked outright.
        Unknown,

        /// Endpoint-independent mapping: peers can reach us directly.
        Open,

        /// Port-dependent mapping (symmetric): direct P2P will not hole-punch.
        Strict,
    };

    struct Result {
        NatType nat = NatType::Unknown;

        /// Best UDP round-trip seen across the probes, in ms; 0 when unreachable.
        int64_t latency_ms = 0;

        bool reachable = false;

        /// External endpoint the servers observed, e.g. "203.0.113.7:39057".
        std::string external_endpoint;

        /// How many of the four probes answered — the honest confidence signal.
        int probes_answered = 0;

        std::string GetLatencyColor() const;
        std::string GetNatColor() const;
    };

    /// Probes both responders on both ports from ONE socket and classifies the NAT from the
    /// external port they each report back.
    static Result Check();

private:
    static constexpr uint32_t TestId = 101;
    static constexpr int ProbeTimeoutMs = 3000;

    /// One probe: send 16 bytes, read the reply, return the external port the server saw.
    static bool Probe(const std::string& host, int port, uint32_t& external_port,
                     int64_t& rtt, std::string& endpoint);
};

} // namespace Common::Nextendo

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_network_check.h"
#include "common/logging.h"
#include <chrono>
#include <thread>
#include <unordered_set>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace Common::Nextendo {

std::string NextendoNetworkCheck::Result::GetLatencyColor() const {
    if (!reachable) {
        return "#E8333E";
    }
    if (latency_ms < 80) {
        return "#33E86B";
    }
    if (latency_ms < 150) {
        return "#E8C83E";
    }
    return "#E8833E";
}

std::string NextendoNetworkCheck::Result::GetNatColor() const {
    switch (nat) {
    case NatType::Open:
        return "#33E86B";
    case NatType::Strict:
        return "#E8833E";
    default:
        return "#808080";
    }
}

NextendoNetworkCheck::Result NextendoNetworkCheck::Check() {
    Result result;

    // The two NAT-check responders, at two DISTINCT addresses
    // Addresses come from environment variables
    const char* nncs1_env = std::getenv("NEXTENDO_SERVER_IP");
    const char* nncs2_env = std::getenv("NEXTENDO_NAT_IP");

    std::string nncs1 = nncs1_env ? nncs1_env : "127.0.0.1";
    std::string nncs2 = nncs2_env ? nncs2_env : "127.0.0.1";

    const int nncs_ports[] = {10025, 10125};

    try {
#ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            LOG_WARNING(Common, "[Nextendo] WSAStartup failed");
            return result;
        }
#endif

        // Create UDP socket
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            LOG_WARNING(Common, "[Nextendo] Failed to create socket");
            return result;
        }

        // Set non-blocking mode
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
#else
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

        // Bind to any available port
        sockaddr_in local_addr{};
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = INADDR_ANY;
        local_addr.sin_port = 0;

        if (bind(sock, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) < 0) {
            LOG_WARNING(Common, "[Nextendo] Failed to bind socket");
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            return result;
        }

        // Prepare probe data (16 bytes with test id)
        uint8_t probe[16] = {0};
        uint32_t test_id_be = htonl(TestId);
        std::memcpy(probe, &test_id_be, sizeof(test_id_be));

        std::unordered_set<uint32_t> external_ports;
        int64_t best_rtt = INT64_MAX;

        // Probe both servers on both ports
        for (const auto& host : {nncs1, nncs2}) {
            for (int port : nncs_ports) {
                uint32_t external_port = 0;
                int64_t rtt = 0;
                std::string endpoint;

                if (Probe(host, port, external_port, rtt, endpoint)) {
                    result.probes_answered++;
                    external_ports.insert(external_port);
                    best_rtt = std::min(best_rtt, rtt);
                    result.external_endpoint = endpoint;
                }
            }
        }

#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif

        if (result.probes_answered == 0) {
            return result;
        }

        result.reachable = true;
        result.latency_ms = best_rtt;

        // One probe answering says nothing about mapping — it takes at least two
        // destinations to compare. Leave it Unknown rather than call it Open.
        if (result.probes_answered < 2) {
            result.nat = NatType::Unknown;
        } else if (external_ports.size() == 1) {
            result.nat = NatType::Open;
        } else {
            result.nat = NatType::Strict;
        }

    } catch (const std::exception& ex) {
        LOG_WARNING(Common, "[Nextendo] NAT check failed: {}", ex.what());
    }

    return result;
}

bool NextendoNetworkCheck::Probe(const std::string& host, int port, uint32_t& external_port,
                                int64_t& rtt, std::string& endpoint) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return false;
    }

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    sockaddr_in target_addr{};
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, host.c_str(), &target_addr.sin_addr);

    // Prepare probe data
    uint8_t probe[16] = {0};
    uint32_t test_id_be = htonl(TestId);
    std::memcpy(probe, &test_id_be, sizeof(test_id_be));

    auto start_time = std::chrono::steady_clock::now();

    sendto(sock, reinterpret_cast<const char*>(probe), sizeof(probe), 0,
           reinterpret_cast<sockaddr*>(&target_addr), sizeof(target_addr));

    // Wait for response with timeout
    uint8_t buffer[64];
    sockaddr_in from_addr{};
    socklen_t from_len = sizeof(from_addr);

    bool received = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ProbeTimeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        int recv_len = recvfrom(sock, reinterpret_cast<char*>(buffer), sizeof(buffer), 0,
                               reinterpret_cast<sockaddr*>(&from_addr), &from_len);

        if (recv_len < 0) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
#endif
            break;
        }

        // Verify the response is from the target server
        if (from_addr.sin_addr.s_addr != target_addr.sin_addr.s_addr ||
            from_addr.sin_port != target_addr.sin_port) {
            continue;
        }

        if (recv_len < 16) {
            continue;
        }

        // Verify the echoed test id
        uint32_t received_id;
        std::memcpy(&received_id, buffer, sizeof(received_id));
        received_id = ntohl(received_id);

        if (received_id != TestId) {
            continue;
        }

        // Parse the response: echoed id, observed source port, observed source IP, server IP
        uint32_t ext_port_be, address_be, server_ip_be;
        std::memcpy(&ext_port_be, buffer + 4, sizeof(ext_port_be));
        std::memcpy(&address_be, buffer + 8, sizeof(address_be));
        std::memcpy(&server_ip_be, buffer + 12, sizeof(server_ip_be));

        external_port = ntohl(ext_port_be);
        uint32_t address = ntohl(address_be);

        // Convert address to string
        struct in_addr addr;
        addr.s_addr = address;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);

        endpoint = std::string(ip_str) + ":" + std::to_string(external_port);

        auto end_time = std::chrono::steady_clock::now();
        rtt = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        received = true;
        break;
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    return received;
}

} // namespace Common::Nextendo

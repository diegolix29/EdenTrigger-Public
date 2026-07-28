// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

namespace Common::Nextendo {

/// Cryptographic utilities for OAuth 2.0 PKCE authentication
class CryptoUtils {
public:
    /// Generate random bytes for PKCE verifier and state
    static std::vector<uint8_t> GenerateRandomBytes(size_t count);

    /// Encode bytes as base64 (URL-safe, no padding)
    static std::string Base64UrlEncode(const std::vector<uint8_t>& data);

    /// Encode bytes as base64 (standard)
    static std::string Base64Encode(const std::vector<uint8_t>& data);

    /// Decode base64 string to bytes
    static std::vector<uint8_t> Base64Decode(const std::string& encoded);

    /// Compute SHA256 hash of data
    static std::vector<uint8_t> SHA256(const std::vector<uint8_t>& data);

    /// Compute SHA256 hash of string
    static std::vector<uint8_t> SHA256(const std::string& data);

    /// Generate a random URL-safe string for PKCE
    static std::string GenerateRandomUrlString(size_t length);
};

} // namespace Common::Nextendo

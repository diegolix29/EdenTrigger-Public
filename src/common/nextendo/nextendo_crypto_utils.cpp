// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_crypto_utils.h"
#include "common/logging.h"
#include <random>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <vector>
#include <stdexcept>

namespace Common::Nextendo {

std::vector<uint8_t> CryptoUtils::GenerateRandomBytes(size_t count) {
    std::vector<uint8_t> bytes(count);
    if (RAND_bytes(bytes.data(), static_cast<int>(count)) != 1) {
        // Fallback to std::random if OpenSSL fails
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(0, 255);
        for (size_t i = 0; i < count; ++i) {
            bytes[i] = static_cast<uint8_t>(dist(gen));
        }
    }
    return bytes;
}

std::string CryptoUtils::Base64UrlEncode(const std::vector<uint8_t>& data) {
    std::string standard = Base64Encode(data);
    // Convert to URL-safe: remove padding, replace '+' with '-', replace '/' with '_'
    std::string url_safe;
    url_safe.reserve(standard.size());
    for (char c : standard) {
        if (c == '+') {
            url_safe += '-';
        } else if (c == '/') {
            url_safe += '_';
        } else if (c != '=') {
            url_safe += c;
        }
    }
    return url_safe;
}

std::string CryptoUtils::Base64Encode(const std::vector<uint8_t>& data) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t chunk = 0;
        chunk |= static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < data.size()) {
            chunk |= static_cast<uint32_t>(data[i + 1]) << 8;
        }
        if (i + 2 < data.size()) {
            chunk |= static_cast<uint32_t>(data[i + 2]);
        }

        result += base64_chars[(chunk >> 18) & 0x3F];
        result += base64_chars[(chunk >> 12) & 0x3F];
        if (i + 1 < data.size()) {
            result += base64_chars[(chunk >> 6) & 0x3F];
        } else {
            result += '=';
        }
        if (i + 2 < data.size()) {
            result += base64_chars[chunk & 0x3F];
        } else {
            result += '=';
        }
    }

    return result;
}

std::vector<uint8_t> CryptoUtils::Base64Decode(const std::string& encoded) {
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> result;

    int in[4] = {0, 0, 0, 0};
    int i = 0;

    for (char c : encoded) {
        if (c == '=') {
            break;
        }
        size_t pos = base64_chars.find(c);
        if (pos == std::string::npos) {
            continue; // Skip non-base64 characters
        }
        in[i++] = static_cast<int>(pos);
        if (i == 4) {
            result.push_back(static_cast<uint8_t>((in[0] << 2) | (in[1] >> 4)));
            result.push_back(static_cast<uint8_t>(((in[1] & 0x0F) << 4) | (in[2] >> 2)));
            result.push_back(static_cast<uint8_t>(((in[2] & 0x03) << 6) | in[3]));
            i = 0;
        }
    }

    if (i > 0) {
        if (i == 3) {
            result.push_back(static_cast<uint8_t>((in[0] << 2) | (in[1] >> 4)));
            result.push_back(static_cast<uint8_t>(((in[1] & 0x0F) << 4) | (in[2] >> 2)));
        } else if (i == 2) {
            result.push_back(static_cast<uint8_t>((in[0] << 2) | (in[1] >> 4)));
        }
    }

    return result;
}

std::vector<uint8_t> CryptoUtils::SHA256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    ::SHA256(data.data(), data.size(), hash.data());
    return hash;
}

std::vector<uint8_t> CryptoUtils::SHA256(const std::string& data) {
    return SHA256(std::vector<uint8_t>(data.begin(), data.end()));
}

std::string CryptoUtils::GenerateRandomUrlString(size_t length) {
    // Generate random bytes and encode as base64 URL-safe
    // For length N, we need approximately N * 0.75 bytes
    size_t byte_count = (length * 3 + 3) / 4;
    std::vector<uint8_t> bytes = GenerateRandomBytes(byte_count);
    std::string encoded = Base64UrlEncode(bytes);
    // Trim to exact length if needed
    if (encoded.length() > length) {
        encoded = encoded.substr(0, length);
    }
    return encoded;
}

} // namespace Common::Nextendo

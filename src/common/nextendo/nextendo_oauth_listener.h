// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <functional>
#include <memory>

namespace Common::Nextendo {

/// OAuth 2.0 callback listener for loopback redirect
/// Handles the HTTP callback from the browser after OAuth authorization
class OAuthListener {
public:
    /// Callback function type for handling the OAuth result
    /// Parameters: code (authorization code), state (CSRF state), error (error message if any)
    using Callback = std::function<void(const std::string& code, const std::string& state, const std::string& error)>;

    OAuthListener();
    ~OAuthListener();

    /// Start the listener on a free loopback port
    /// Returns the port number, or 0 on failure
    int Start();

    /// Stop the listener
    void Stop();

    /// Set the callback to handle the OAuth result
    void SetCallback(Callback callback);

    /// Get the redirect URI for this listener
    std::string GetRedirectUri() const;

    /// Wait for the callback (with timeout in seconds)
    /// Returns true if callback was received, false on timeout
    bool WaitForCallback(int timeout_seconds = 300);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace Common::Nextendo

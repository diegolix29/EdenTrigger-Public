// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_oauth_listener.h"
#include "common/logging.h"
#include "common/httplib.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace Common::Nextendo {

struct OAuthListener::Impl {
    std::unique_ptr<httplib::Server> server;
    int port = 0;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> callback_received{false};
    std::string code;
    std::string state;
    std::string error;
    Callback callback;
    std::thread server_thread;

    Impl() = default;
    ~Impl() {
        Stop();
    }

    void Stop() {
        if (server) {
            server->stop();
            server = nullptr;
        }
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

OAuthListener::OAuthListener() : pImpl(std::make_unique<Impl>()) {}

OAuthListener::~OAuthListener() {
    Stop();
}

int OAuthListener::Start() {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    // Find a free loopback port
    for (int attempt = 0; attempt < 10; ++attempt) {
        int test_port = 49152 + attempt; // Start from dynamic port range
        pImpl->server = std::make_unique<httplib::Server>();

        // Set up the callback handler
        pImpl->server->Get("/callback", [this](const httplib::Request& req, httplib::Response& res) {
            LOG_INFO(Common, "[Nextendo] OAuth callback received");

            std::string code = req.get_param_value("code");
            std::string state = req.get_param_value("state");
            std::string error = req.get_param_value("error");

            // Send HTML response to the browser
            std::string html;
            if (error.empty() && !code.empty()) {
                html = R"(<!doctype html><meta charset=utf-8><title>Nextendo</title>)"
                       R"(<body style='font-family:system-ui,sans-serif;background:#0f1115;color:#e7e9ee;display:grid;place-items:center;height:100vh;margin:0'>)"
                       R"(<div style='text-align:center;max-width:420px'>)"
                       R"(<h1 style='color:#33E86B'>✓ Connexion réussie</h1>)"
                       R"(<p>Tu peux fermer cet onglet et retourner à l'émulateur Nextendo.</p>)"
                       R"(</div>)";
            } else {
                html = R"(<!doctype html><meta charset=utf-8><title>Nextendo</title>)"
                       R"(<body style='font-family:system-ui,sans-serif;background:#0f1115;color:#e7e9ee;display:grid;place-items:center;height:100vh;margin:0'>)"
                       R"(<div style='text-align:center;max-width:420px'>)"
                       R"(<h1 style='color:#ff8a8a'>Connexion annulée</h1>)"
                       R"(<p>Retourne à l'émulateur Nextendo et réessaie.</p>)"
                       R"(</div>)";
            }

            res.set_content(html, "text/html; charset=utf-8");

            // Store the callback data
            {
                std::lock_guard<std::mutex> lock(pImpl->mutex);
                pImpl->code = code;
                pImpl->state = state;
                pImpl->error = error;
                pImpl->callback_received = true;
            }

            // Notify waiting thread
            pImpl->cv.notify_one();

            // Call user callback if set
            if (pImpl->callback) {
                pImpl->callback(code, state, error);
            }
        });

        // Try to bind to the port
        if (pImpl->server->bind_to_port("127.0.0.1", test_port)) {
            pImpl->port = test_port;
            // Start the server in a separate thread
            pImpl->server_thread = std::thread([this]() {
                pImpl->server->listen_after_bind();
            });
            LOG_INFO(Common, "[Nextendo] OAuth listener started on port {}", pImpl->port);
            return pImpl->port;
        }

        // Port in use, try next
        pImpl->server = nullptr;
    }

    LOG_ERROR(Common, "[Nextendo] Failed to find free loopback port for OAuth listener");
    return 0;
}

void OAuthListener::Stop() {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->Stop();
}

void OAuthListener::SetCallback(Callback callback) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->callback = callback;
}

std::string OAuthListener::GetRedirectUri() const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    if (pImpl->port == 0) {
        return "";
    }
    return "http://127.0.0.1:" + std::to_string(pImpl->port) + "/callback";
}

bool OAuthListener::WaitForCallback(int timeout_seconds) {
    std::unique_lock<std::mutex> lock(pImpl->mutex);
    return pImpl->cv.wait_for(lock, std::chrono::seconds(timeout_seconds),
                              [this] { return pImpl->callback_received.load(); });
}

} // namespace Common::Nextendo

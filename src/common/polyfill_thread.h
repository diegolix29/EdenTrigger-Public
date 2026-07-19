// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

//
// TODO: remove this file when jthread is supported by all compilation targets
//

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

namespace Common {

// Apple Clang (__apple_build_version__) never implemented std::jthread /
// std::stop_token, even though older SDKs' __has_include(<stop_token>) can
// be misleading. Non-Apple-Clang toolchains targeting macOS (e.g. Homebrew
// LLVM) ship a real implementation, so only special-case actual AppleClang.
#if __has_include(<stop_token>) && !defined(__apple_build_version__)
#include <stop_token>
using std::stop_token;
#else
// Polyfill for std::stop_token for platforms that don't have it yet
class stop_token {
public:
    stop_token() noexcept = default;
    stop_token(const stop_token&) noexcept = default;
    stop_token(stop_token&&) noexcept = default;
    stop_token& operator=(const stop_token&) noexcept = default;
    stop_token& operator=(stop_token&&) noexcept = default;
    ~stop_token() = default;

    bool stop_requested() const noexcept {
        return state && state->stop_requested.load(std::memory_order_acquire);
    }

private:
    struct stop_state {
        std::atomic<bool> stop_requested{false};
    };
    std::shared_ptr<stop_state> state;
};
#endif

template <typename Rep, typename Period>
bool StoppableTimedWait(stop_token token, const std::chrono::duration<Rep, Period>& rel_time) {
    std::condition_variable_any cv;
    std::mutex m;

    // Perform the timed wait.
    std::unique_lock lk{m};
    return !cv.wait_for(lk, token, rel_time, [&] { return token.stop_requested(); });
}

} // namespace Common
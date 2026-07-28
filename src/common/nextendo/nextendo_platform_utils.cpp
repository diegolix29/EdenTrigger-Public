// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/nextendo/nextendo_platform_utils.h"
#include "common/logging.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>
#include <ApplicationServices/ApplicationServices.h>
#else
#include <cstdlib>
#endif

namespace Common::Nextendo {

bool PlatformUtils::OpenUrl(const std::string& url) {
#ifdef _WIN32
    // Windows: Use ShellExecute
    HINSTANCE result = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOW);
    return reinterpret_cast<intptr_t>(result) > 32;
#elif defined(__APPLE__)
    // macOS: Use LSOpenCFURLRef
    CFURLRef cf_url = CFURLCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(url.c_str()),
                                          url.length(), kCFStringEncodingUTF8, nullptr);
    if (!cf_url) {
        return false;
    }
    OSStatus status = LSOpenCFURLRef(cf_url, nullptr);
    CFRelease(cf_url);
    return status == noErr;
#else
    // Linux/Unix: Use xdg-open
    std::string command = "xdg-open " + url;
    int result = std::system(command.c_str());
    return result == 0;
#endif
}

} // namespace Common::Nextendo

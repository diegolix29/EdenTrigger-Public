// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.features.settings.model

import android.content.Context
import org.yuzu.yuzu_emu.YuzuApplication

/**
 * SharedPreferences-based settings for Memory Flush feature
 * to avoid native configuration system crashes
 */
object MemoryFlushSettings {
    private const val PREFS_NAME = "memory_flush_prefs"
    private const val KEY_ENABLE_MEMORY_FLUSH = "enable_memory_flush"
    private const val KEY_SHOW_MEMORY_FLUSH_BUTTON = "show_memory_flush_button"
    private val prefs = YuzuApplication.appContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    fun isMemoryFlushEnabled(): Boolean = prefs.getBoolean(KEY_ENABLE_MEMORY_FLUSH, false)
    fun setMemoryFlushEnabled(enabled: Boolean) {
        prefs.edit().putBoolean(KEY_ENABLE_MEMORY_FLUSH, enabled).apply()
    }
    fun shouldShowMemoryFlushButton(): Boolean = prefs.getBoolean(KEY_SHOW_MEMORY_FLUSH_BUTTON, false)
    fun setShowMemoryFlushButton(show: Boolean) {
        prefs.edit().putBoolean(KEY_SHOW_MEMORY_FLUSH_BUTTON, show).apply()
    }
}
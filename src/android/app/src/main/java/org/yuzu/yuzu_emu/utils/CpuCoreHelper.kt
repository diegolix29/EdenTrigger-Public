// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.ActivityManager
import android.content.Context
import android.os.Build
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.NativeLibrary
import java.io.File
import java.io.IOException

data class CpuCore(
    val id: Int,
    val name: String,
    val isPerformance: Boolean,
    val isEfficiency: Boolean,
    val maxFrequency: Int = 0,
    val isActive: Boolean = true
)

object CpuCoreHelper {

    private val context get() = YuzuApplication.appContext

    fun getAvailableCores(): List<CpuCore> {
        val cores = mutableListOf<CpuCore>()
        val numCores = Runtime.getRuntime().availableProcessors()

        for (i in 0 until numCores) {
            val coreName = getCpuCoreName(i)
            val maxFreq = getCpuMaxFrequency(i)
            val isPerformance = isPerformanceCore(i, maxFreq)
            val isEfficiency = isEfficiencyCore(i, maxFreq)

            cores.add(
                CpuCore(
                    id = i,
                    name = coreName,
                    isPerformance = isPerformance,
                    isEfficiency = isEfficiency,
                    maxFrequency = maxFreq
                )
            )
        }

        return cores
    }

    private fun getCpuCoreName(coreId: Int): String {
        return try {
            // Try to read from sysfs
            val coreInfoFile = File("/sys/devices/system/cpu/cpu$coreId/cpufreq/cpuinfo_max_freq")
            if (coreInfoFile.exists()) {
                "Core $coreId"
            } else {
                // Fallback to generic naming
                when {
                    coreId < 4 -> "Performance Core $coreId"
                    else -> "Efficiency Core $coreId"
                }
            }
        } catch (e: Exception) {
            "Core $coreId"
        }
    }

    private fun getCpuMaxFrequency(coreId: Int): Int {
        return try {
            val freqFile = File("/sys/devices/system/cpu/cpu$coreId/cpufreq/cpuinfo_max_freq")
            if (freqFile.exists() && freqFile.canRead()) {
                freqFile.readText().trim().toIntOrNull() ?: 0
            } else {
                0
            }
        } catch (e: Exception) {
            0
        }
    }

    private fun isPerformanceCore(coreId: Int, maxFreq: Int): Boolean {
        // Simple heuristic based on core ID and frequency
        // On most devices, cores 0-3 are performance cores
        return when {
            maxFreq > 0 -> maxFreq > 2000000 // > 2GHz likely performance
            coreId < 4 -> true // First 4 cores usually performance
            else -> false
        }
    }

    private fun isEfficiencyCore(coreId: Int, maxFreq: Int): Boolean {
        return when {
            maxFreq > 0 -> maxFreq < 2000000 // < 2GHz likely efficiency
            coreId >= 4 -> true // Later cores usually efficiency
            else -> false
        }
    }

    fun getCustomCoreSelection(): Set<Int> {
        // Read from common settings instead of SharedPreferences
        val customCores = NativeConfig.getCpuCustomCores(!NativeConfig.isPerGameConfigLoaded())
        val coreSet = mutableSetOf<Int>()
        for (i in 0 until 64) {
            if ((customCores and (1L shl i)) != 0L) {
                coreSet.add(i)
            }
        }
        return coreSet
    }

    fun setCustomCoreSelection(coreIds: Set<Int>) {
        // Convert core IDs to bitset and store in common settings
        var customCores = 0L
        for (coreId in coreIds) {
            customCores = customCores or (1L shl coreId)
        }
        NativeConfig.setCpuCustomCores(customCores)

        // Apply the CPU core affinity immediately
        applyCpuCoreAffinity(coreIds)
    }

    fun getActiveCoresForConfig(config: Int): Set<Int> {
        return when (config) {
            0 -> getAvailableCores().map { it.id }.toSet() // All cores
            1 -> getAvailableCores().filter { it.isEfficiency }.map { it.id }.toSet() // Efficiency only
            2 -> getAvailableCores().filter { it.isPerformance }.map { it.id }.toSet() // Performance only
            3 -> getCustomCoreSelection() // Custom selection
            else -> getAvailableCores().map { it.id }.toSet() // Default to all
        }
    }

    /**
     * Applies CPU core affinity for the current configuration
     */
    fun applyCurrentCpuCoreConfig() {
        val config = org.yuzu.yuzu_emu.features.settings.model.IntSetting.CPU_CORE_CONFIG.getInt(
            !org.yuzu.yuzu_emu.utils.NativeConfig.isPerGameConfigLoaded()
        )
        val activeCores = getActiveCoresForConfig(config)
        applyCpuCoreAffinity(activeCores)
    }

    /**
     * Applies CPU core affinity using native method
     */
    private fun applyCpuCoreAffinity(coreIds: Set<Int>) {
        try {
            if (coreIds.isNotEmpty()) {
                NativeLibrary.setCpuCoreAffinity(coreIds.toIntArray())
            }
        } catch (e: Exception) {
            // Log error but don't crash
            android.util.Log.e("CpuCoreHelper", "Failed to apply CPU core affinity", e)
        }
    }
}

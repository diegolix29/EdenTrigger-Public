// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import java.io.BufferedReader
import java.io.File
import java.io.FileReader
import java.io.IOException
import kotlin.math.roundToInt

object TemperatureMonitor {
    private const val THERMAL_ZONE_PATH = "/sys/class/thermal/"
    private const val CPU_TEMP_PREFIX = "thermal_zone"
    private const val TEMP_FILE_SUFFIX = "/temp"
    // Common CPU thermal zones
    private val CPU_THERMAL_ZONES = listOf(
        "cpu-thermal",
        "cpu_thermal",
        "soc_thermal",
        "tsens_tz_sensor",
        "tsens_tz_sensor0",
        "tsens_tz_sensor1",
        "tsens_tz_sensor2",
        "tsens_tz_sensor3",
        "tsens_tz_sensor4",
        "tsens_tz_sensor5",
        "tsens_tz_sensor6",
        "tsens_tz_sensor7",
        "tsens_tz_sensor8",
        "tsens_tz_sensor9",
        "tsens_tz_sensor10",
        "tsens_tz_sensor11",
        "tsens_tz_sensor12",
        "tsens_tz_sensor13",
        "tsens_tz_sensor14",
        "tsens_tz_sensor15"
    )
    fun getCpuTemperature(): Float {
        return try {
            // Read from sysfs for temperature monitoring
            getCpuTemperatureFromSysfs()
        } catch (e: Exception) {
            Log.error("[TemperatureMonitor] Failed to get CPU temperature: ${e.message}")
            -1.0f
        }
    }
    private fun getCpuTemperatureFromSysfs(): Float {
        val thermalDir = File(THERMAL_ZONE_PATH)
        if (!thermalDir.exists()) {
            return -1.0f
        }
        // First try to find CPU-specific thermal zones
        for (zone in CPU_THERMAL_ZONES) {
            val temp = readTemperatureFromZone(zone)
            if (temp > 0) {
                return temp
            }
        }
        // If no specific zones found, try all thermal zones and get the highest
        var maxTemp = 0f
        thermalDir.listFiles()?.forEach { file ->
            if (file.name.startsWith(CPU_TEMP_PREFIX) && file.isDirectory) {
                val temp = readTemperatureFromZone(file.name)
                if (temp > maxTemp) {
                    maxTemp = temp
                }
            }
        }
        return if (maxTemp > 0) maxTemp else -1.0f
    }

    private fun readTemperatureFromZone(zoneName: String): Float {
        return try {
            val tempFile = File("$THERMAL_ZONE_PATH$zoneName$TEMP_FILE_SUFFIX")
            if (!tempFile.exists() || !tempFile.canRead()) {
                return -1.0f
            }
            BufferedReader(FileReader(tempFile)).use { reader ->
                val line = reader.readLine()
                line?.let {
                    // Temperature is usually in millidegrees Celsius
                    val temp = it.toFloatOrNull()
                    temp?.let { temp / 1000f } ?: -1.0f
                } ?: -1.0f
            }
        } catch (e: IOException) {
            -1.0f
        }
    }
    fun getFormattedTemperature(): String {
        val temp = getCpuTemperature()
        return if (temp > 0) {
            "${temp.roundToInt()}°C"
        } else {
            "N/A"
        }
    }
    fun isTemperatureAvailable(): Boolean {
        return try {
            val thermalDir = File(THERMAL_ZONE_PATH)
            thermalDir.exists() && thermalDir.listFiles()?.any {
                it.name.startsWith(CPU_TEMP_PREFIX) && it.isDirectory
            } == true
        } catch (e: Exception) {
            false
        }
    }
}
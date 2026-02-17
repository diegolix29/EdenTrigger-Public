// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.overlay

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.drawable.Drawable
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import android.widget.FrameLayout
import android.widget.Toast
import androidx.core.content.ContextCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.MemoryFlushSettings
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.NativeLibrary

class MemoryFlushOverlayButton @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var buttonDrawable: Drawable? = null
    private val paint = Paint()
    private var isPressed = false
    private val coroutineScope = CoroutineScope(Dispatchers.Main)
    private var backgroundFlushJob: Job? = null
    init {
        init()
    }
    fun startBackgroundMemoryFlush() {
        stopBackgroundMemoryFlush() // Stop any existing job

        // Background flush only depends on the "enable memory flush" setting
        if (MemoryFlushSettings.isMemoryFlushEnabled()) {
            backgroundFlushJob = coroutineScope.launch(Dispatchers.IO) {
                while (isActive) {
                    // Check every 5 minutes
                    delay(5 * 60 * 1000L)

                    // Only run background flush if the setting is enabled and emulation is running
                    if (MemoryFlushSettings.isMemoryFlushEnabled() && NativeLibrary.isRunning()) {
                        performBackgroundMemoryFlush()
                    }
                }
            }
        }
    }
    fun stopBackgroundMemoryFlush() {
        backgroundFlushJob?.cancel()
        backgroundFlushJob = null
    }

    private fun init() {
        buttonDrawable = ContextCompat.getDrawable(context, R.drawable.ic_delete)
        paint.color = ContextCompat.getColor(context, android.R.color.white)
        paint.alpha = 200
        paint.isAntiAlias = true

        // Set button size - smaller now
        val buttonSize = (60 * resources.displayMetrics.density).toInt()
        layoutParams = FrameLayout.LayoutParams(buttonSize, buttonSize).apply {
            // Position button in bottom-right corner
            val margin = (16 * resources.displayMetrics.density).toInt()
            setMargins(margin, 0, margin, margin)
            gravity = android.view.Gravity.BOTTOM or android.view.Gravity.END
        }
        // Initially hide the button
        visibility = GONE
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        buttonDrawable?.let { drawable ->
            // Draw button background
            if (isPressed) {
                paint.alpha = 150
            } else {
                paint.alpha = 200
            }
            val radius = width / 2f
            canvas.drawCircle(width / 2f, height / 2f, radius, paint)
            // Draw icon
            val iconSize = (width * 0.6f).toInt()
            val iconLeft = (width - iconSize) / 2
            val iconTop = (height - iconSize) / 2
            drawable.setBounds(iconLeft, iconTop, iconLeft + iconSize, iconTop + iconSize)
            drawable.draw(canvas)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                isPressed = true
                invalidate()
                return true
            }
            MotionEvent.ACTION_UP -> {
                isPressed = false
                invalidate()
                performMemoryFlush()
                return true
            }
            MotionEvent.ACTION_CANCEL -> {
                isPressed = false
                invalidate()
                return true
            }
        }
        return false
    }

    private fun performMemoryFlush() {
        performMemoryFlushInternal(showToast = true)
    }

    private fun performBackgroundMemoryFlush() {
        performMemoryFlushInternal(showToast = false)
    }
    private fun performMemoryFlushInternal(showToast: Boolean) {
        try {
            // Button should work regardless of background setting - it's manual/on-demand
            coroutineScope.launch(Dispatchers.IO) {
                try {
                    // Get initial memory state
                    val runtime = Runtime.getRuntime()
                    val beforeMemory = runtime.totalMemory() - runtime.freeMemory()

                    // Trigger Java garbage collection
                    System.gc()
                    System.runFinalization()

                    // Give GC a moment to work
                    Thread.sleep(100)

                    // Check memory after GC
                    System.gc()

                    // If native library is running and has memory flush functionality
                    if (NativeLibrary.isRunning()) {
                        try {
                            // Attempt to flush native memory if function exists
                            // This would need to be implemented in the native code
                            // NativeLibrary.flushMemory()
                        } catch (e: Exception) {
                            // Native memory flush not available, continue with Java GC only
                        }
                    }

                    // Calculate memory freed
                    val afterMemory = runtime.totalMemory() - runtime.freeMemory()
                    val memoryFreed = beforeMemory - afterMemory

                    if (showToast) {
                        val message = if (memoryFreed > 0) {
                            val memoryFreedMB = memoryFreed / (1024 * 1024)
                            context.getString(R.string.memory_flush_completed_with_amount, memoryFreedMB)
                        } else {
                            context.getString(R.string.memory_flush_completed)
                        }

                        // Show toast on main thread
                        launch(Dispatchers.Main) {
                            Toast.makeText(context, message, Toast.LENGTH_SHORT).show()
                        }
                    }

                } catch (e: Exception) {
                    if (showToast) {
                        launch(Dispatchers.Main) {
                            Toast.makeText(
                                context,
                                context.getString(R.string.memory_flush_error),
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                    }
                }
            }
        } catch (e: Exception) {
            // If settings are not available, show error
            if (showToast) {
                Toast.makeText(
                    context,
                    context.getString(R.string.memory_flush_error),
                    Toast.LENGTH_SHORT
                ).show()
            }
        }
    }

    fun updateVisibility() {
        try {
            // Button visibility only depends on "show button" setting, not on memory flush being enabled
            visibility = if (MemoryFlushSettings.shouldShowMemoryFlushButton()) {
                VISIBLE
            } else {
                GONE
            }
        } catch (e: Exception) {
            // If settings are not available, hide the button
            visibility = GONE
        }
    }
}
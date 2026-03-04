// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.dialogs

import android.app.Dialog
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.TextView
import androidx.fragment.app.DialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.utils.CpuCoreHelper
import org.yuzu.yuzu_emu.utils.CpuCore

class CpuCoreSelectionDialogFragment : DialogFragment() {

    private lateinit var coreCheckboxes: MutableList<CheckBox>
    private val availableCores by lazy { CpuCoreHelper.getAvailableCores() }
    private var onSelectionChanged: ((Set<Int>) -> Unit)? = null

    fun setOnSelectionChanged(listener: (Set<Int>) -> Unit) {
        onSelectionChanged = listener
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val context = requireContext()
        val inflater = LayoutInflater.from(context)
        val view = inflater.inflate(R.layout.dialog_cpu_core_selection, null)

        setupCoreSelection(view)

        return MaterialAlertDialogBuilder(context)
            .setTitle(R.string.cpu_core_custom_selection)
            .setView(view)
            .setPositiveButton(android.R.string.ok) { _, _ ->
                saveSelection()
            }
            .setNegativeButton(android.R.string.cancel, null)
            .create()
    }

    private fun setupCoreSelection(view: View) {
        val container = view.findViewById<ViewGroup>(R.id.core_selection_container)
        coreCheckboxes = mutableListOf()

        val currentSelection = CpuCoreHelper.getCustomCoreSelection()

        availableCores.forEach { core ->
            val inflater = LayoutInflater.from(requireContext())
            val coreView = inflater.inflate(R.layout.item_cpu_core, container, false)

            val checkbox = coreView.findViewById<CheckBox>(R.id.core_checkbox).apply {
                text = core.name
                isChecked = currentSelection.contains(core.id)
                tag = core.id
            }

            val typeLabel = coreView.findViewById<TextView>(R.id.core_type_label)
            val freqLabel = coreView.findViewById<TextView>(R.id.core_frequency_label)

            typeLabel.text = when {
                core.isPerformance -> getString(R.string.performance_core)
                core.isEfficiency -> getString(R.string.efficiency_core)
                else -> getString(R.string.standard_core)
            }

            if (core.maxFrequency > 0) {
                freqLabel.text = getString(R.string.core_frequency, core.maxFrequency / 1000)
                freqLabel.visibility = View.VISIBLE
            } else {
                freqLabel.visibility = View.GONE
            }

            coreCheckboxes.add(checkbox)
            container.addView(coreView)
        }

        // Add select all/none buttons
        val selectAllButton = view.findViewById<View>(R.id.btn_select_all)
        val selectNoneButton = view.findViewById<View>(R.id.btn_select_none)

        selectAllButton?.setOnClickListener {
            coreCheckboxes.forEach { it.isChecked = true }
        }

        selectNoneButton?.setOnClickListener {
            coreCheckboxes.forEach { it.isChecked = false }
        }
    }

    private fun saveSelection() {
        val selectedCores = coreCheckboxes
            .filter { it.isChecked }
            .map { it.tag as Int }
            .toSet()

        CpuCoreHelper.setCustomCoreSelection(selectedCores)
        onSelectionChanged?.invoke(selectedCores)
    }

    companion object {
        fun newInstance(): CpuCoreSelectionDialogFragment {
            return CpuCoreSelectionDialogFragment()
        }
    }
}

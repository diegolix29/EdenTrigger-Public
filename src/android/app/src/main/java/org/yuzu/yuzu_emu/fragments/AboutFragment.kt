// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.navigation.findNavController
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.transition.MaterialSharedAxis
import org.yuzu.yuzu_emu.HomeNavigationDirections
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.FragmentAboutBinding
import org.yuzu.yuzu_emu.features.settings.ui.SettingsSubscreen
import org.yuzu.yuzu_emu.model.HomeViewModel
import org.yuzu.yuzu_emu.utils.ViewUtils.updateMargins
import org.yuzu.yuzu_emu.NativeLibrary

class AboutFragment : Fragment() {
    private var _binding: FragmentAboutBinding? = null
    private val binding get() = _binding!!

    private val homeViewModel: HomeViewModel by activityViewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enterTransition = MaterialSharedAxis(MaterialSharedAxis.X, true)
        returnTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)
        reenterTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentAboutBinding.inflate(layoutInflater)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        homeViewModel.setStatusBarShadeVisibility(visible = false)
        binding.toolbarAbout.setNavigationOnClickListener {
            requireActivity().onBackPressedDispatcher.onBackPressed()
        }

        binding.imageLogo.setOnLongClickListener {
            Toast.makeText(
                requireContext(),
                R.string.gaia_is_not_real,
                Toast.LENGTH_SHORT
            ).show()
            true
        }

        binding.buttonContributors.setOnClickListener {
            openLink(
                getString(R.string.contributors_link)
            )
        }
        binding.buttonLicenses.setOnClickListener {
            val action = HomeNavigationDirections.actionGlobalSettingsSubscreenActivity(
                SettingsSubscreen.LICENSES,
                null
            )
            binding.root.findNavController().navigate(action)
        }

        val buildName = getString(R.string.app_name_suffixed)
        val buildVersion = NativeLibrary.getBuildVersion()
        val fullVersionText = "$buildName ($buildVersion)"

        binding.textVersionName.text = fullVersionText
        binding.buttonVersionName.setOnClickListener {
            val clipBoard =
                requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
            val clip = ClipData.newPlainText(getString(R.string.build), fullVersionText)
            clipBoard.setPrimaryClip(clip)

            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
                Toast.makeText(
                    requireContext(),
                    R.string.copied_to_clipboard,
                    Toast.LENGTH_SHORT
                ).show()
            }
        }

        binding.buttonDiscord.setOnClickListener { openLink(getString(R.string.discord_link)) }
        binding.buttonStoat.setOnClickListener { openLink(getString(R.string.stoat_link)) }
        binding.buttonX.setOnClickListener { openLink(getString(R.string.x_link)) }
        binding.buttonWebsite.setOnClickListener { openLink(getString(R.string.website_link)) }
        binding.buttonGithub.setOnClickListener { openLink(getString(R.string.github_link)) }
        binding.buttonCheckUpdates.setOnClickListener { checkForUpdates() }

        setInsets()
    }

    private fun openLink(link: String) {
        val intent = Intent(Intent.ACTION_VIEW, Uri.parse(link))
        startActivity(intent)
    }

    private fun checkForUpdates() {
        if (!NativeLibrary.isUpdateCheckerEnabled()) {
            MaterialAlertDialogBuilder(requireContext())
                .setTitle(getString(R.string.check_for_updates))
                .setMessage(getString(R.string.update_check_disabled))
                .setPositiveButton(android.R.string.ok, null)
                .show()
            return
        }

        Thread {
            val updateResult = NativeLibrary.checkForUpdate()
            requireActivity().runOnUiThread {
                if (updateResult != null) {
                    showUpdateDialog(updateResult)
                } else {
                    MaterialAlertDialogBuilder(requireContext())
                        .setTitle(getString(R.string.check_for_updates))
                        .setMessage(getString(R.string.no_update_available))
                        .setPositiveButton(android.R.string.ok, null)
                        .show()
                }
            }
        }.start()
    }

    private fun showUpdateDialog(updateResult: NativeLibrary.UpdateResult) {
        val message = buildString {
            append(updateResult.title)
            append("\n\n")
            append(updateResult.body)
        }

        MaterialAlertDialogBuilder(requireContext())
            .setTitle(getString(R.string.update_available))
            .setMessage(message)
            .setPositiveButton(android.R.string.ok) { _, _ ->
                downloadAndInstallUpdate(updateResult)
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    private fun downloadAndInstallUpdate(updateResult: NativeLibrary.UpdateResult) {
        // Find the APK URL from assets
        val apkUrl = findApkUrl(updateResult)
        if (apkUrl.isNullOrEmpty()) {
            MaterialAlertDialogBuilder(requireContext())
                .setTitle(getString(R.string.update_available))
                .setMessage("No APK found in update assets. Opening download page instead.")
                .setPositiveButton(android.R.string.ok) { _, _ ->
                    openLink(updateResult.url)
                }
                .setNegativeButton(android.R.string.cancel, null)
                .show()
            return
        }

        // Download APK
        Thread {
            try {
                val url = java.net.URL(apkUrl)
                val connection = url.openConnection() as java.net.HttpURLConnection
                connection.connect()

                val apkFile = java.io.File(
                    requireContext().getExternalFilesDir(null),
                    "eden_update.apk"
                )
                val input = connection.inputStream
                val output = java.io.FileOutputStream(apkFile)
                input.copyTo(output)
                input.close()
                output.close()

                requireActivity().runOnUiThread {
                    installApk(apkFile)
                }
            } catch (e: Exception) {
                requireActivity().runOnUiThread {
                    MaterialAlertDialogBuilder(requireContext())
                        .setTitle(getString(R.string.update_available))
                        .setMessage("Failed to download update: ${e.message}")
                        .setPositiveButton(android.R.string.ok, null)
                        .show()
                }
            }
        }.start()
    }

    private fun findApkUrl(updateResult: NativeLibrary.UpdateResult): String? {
        // Look for .apk file in assets
        for (asset in updateResult.assets) {
            if (asset.endsWith(".apk")) {
                return asset
            }
        }
        return null
    }

    private fun installApk(apkFile: java.io.File) {
        val apkUri = androidx.core.content.FileProvider.getUriForFile(
            requireContext(),
            "${requireContext().packageName}.provider",
            apkFile
        )

        val intent = android.content.Intent(android.content.Intent.ACTION_VIEW).apply {
            setDataAndType(apkUri, "application/vnd.android.package-archive")
            flags = android.content.Intent.FLAG_ACTIVITY_NEW_TASK or
                    android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION
        }
        // For Android 7.0+, grant read permission to the content URI
        val resInfoList = requireContext().packageManager.queryIntentActivities(intent, android.content.pm.PackageManager.MATCH_DEFAULT_ONLY)
        for (resolveInfo in resInfoList) {
            requireContext().grantUriPermission(
                resolveInfo.activityInfo.packageName,
                apkUri,
                android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION
            )
        }
        startActivity(intent)
    }

    private fun setInsets() =
        ViewCompat.setOnApplyWindowInsetsListener(
            binding.root
        ) { _: View, windowInsets: WindowInsetsCompat ->
            val barInsets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            val cutoutInsets = windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout())

            val leftInsets = barInsets.left + cutoutInsets.left
            val rightInsets = barInsets.right + cutoutInsets.right

            binding.toolbarAbout.updateMargins(left = leftInsets, right = rightInsets)
            binding.scrollAbout.updateMargins(left = leftInsets, right = rightInsets)

            binding.contentAbout.updatePadding(bottom = barInsets.bottom)

            windowInsets
        }
}

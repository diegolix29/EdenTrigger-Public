// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QIcon>
#include <QMessageBox>
#include <QtConcurrent>
#include <fmt/ranges.h>
#include "common/scm_rev.h"
#include "ui_aboutdialog.h"
#include "yuzu/about_dialog.h"

#ifdef ENABLE_UPDATE_CHECKER
#include "frontend_common/update_checker.h"
#include "yuzu/updater/update_dialog.h"
#endif

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()} {
    static const std::string build_id = std::string{Common::g_build_id};
    static const std::string yuzu_build =
        fmt::format("{} | {} | {}", std::string{Common::g_build_name},
                    std::string{Common::g_build_version}, std::string{Common::g_compiler_id});

    const auto override_build =
        fmt::format(fmt::runtime(std::string(Common::g_title_bar_format_idle)), build_id);
    const auto yuzu_build_version = override_build.empty() ? yuzu_build : override_build;

    ui->setupUi(this);
    // Try and request the icon from Qt theme (Linux?)
    const QIcon yuzu_logo = QIcon::fromTheme(QStringLiteral("org.yuzu_emu.yuzu"));
    if (!yuzu_logo.isNull()) {
        ui->labelLogo->setPixmap(yuzu_logo.pixmap(200));
    }
    ui->labelBuildInfo->setText(
        ui->labelBuildInfo->text().arg(QString::fromStdString(yuzu_build_version),
                                       QString::fromUtf8(Common::g_build_date).left(10)));

    connect(ui->buttonCheckForUpdates, &QPushButton::clicked, this, &AboutDialog::OnCheckForUpdates);
}

AboutDialog::~AboutDialog() = default;

#ifdef ENABLE_UPDATE_CHECKER
void AboutDialog::OnCheckForUpdates() {
    auto future = QtConcurrent::run([]() -> std::optional<Common::Net::Release> {
        return UpdateChecker::GetUpdate();
    });
    auto* watcher = new QFutureWatcher<std::optional<Common::Net::Release>>(this);
    watcher->setFuture(future);
    connect(watcher, &QFutureWatcher<std::optional<Common::Net::Release>>::finished, this,
            [this, watcher]() {
                watcher->deleteLater();
                auto result = watcher->result();
                if (result) {
                    UpdateDialog dialog(result.value(), this);
                    dialog.exec();
                } else {
                    QMessageBox::information(this, tr("No Update Available"),
                                           tr("You are already using the latest version."));
                }
            });
}
#else
void AboutDialog::OnCheckForUpdates() {
    QMessageBox::information(this, tr("Update Check Disabled"),
                             tr("Update checking is disabled in this build."));
}
#endif

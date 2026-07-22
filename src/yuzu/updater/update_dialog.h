// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QNetworkAccessManager>
#include "common/net/net.h"

class QRadioButton;
namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpdateDialog(const Common::Net::Release& release, QWidget* parent = nullptr);
    ~UpdateDialog();

    void accept() override;

private slots:
    void Download();

private:
    void Install(const QString& tempDirPath);
    void ApplyModernStyle();
#ifndef _WIN32
    QString WriteInstallScript(const QString& tempDirPath, const QString& rootPath);
#endif

    Ui::UpdateDialog* ui;
    QList<QRadioButton*> m_buttons;
    Common::Net::Asset m_asset;
    Common::Net::Release m_release;
    QNetworkAccessManager* networkManager;

    // Full path to the downloaded archive, including the *correct* extension
    // for whatever asset was actually downloaded (.zip, .tar.gz, ...).
    QString m_downloadPath;
    QString m_archiveExtension;
};
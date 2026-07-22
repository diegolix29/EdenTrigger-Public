// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressBar>
#include <QRadioButton>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVBoxLayout>
#include "common/logging.h"
#include "common/scm_rev.h"
#include "qt_common/abstract/frontend.h"
#include "qt_common/abstract/progress.h"
#include "ui_update_dialog.h"
#include "update_dialog.h"

#include "common/httplib.h"

#ifdef YUZU_BUNDLED_OPENSSL
#include <openssl/cert.h>
#endif

#include <QDesktopServices>

#undef GetSaveFileName

UpdateDialog::UpdateDialog(const Common::Net::Release& release, QWidget* parent)
    : QDialog(parent), ui(new Ui::UpdateDialog), m_buttons(), m_asset(), m_release(release),
      networkManager(new QNetworkAccessManager(this)) {
    ui->setupUi(this);

    ApplyModernStyle();

    QString titleText = QString::fromUtf8("%1 is available for download.");
    titleText.replace(QLatin1String("%1"), QString::fromStdString(release.title));
    ui->version->setText(titleText);

    QString urlText = QString::fromUtf8("<a href=\"%1\">View on GitHub</a>");
    urlText.replace(QLatin1String("%1"), QString::fromStdString(release.html_url));
    ui->url->setText(urlText);

    std::string text{release.body};
    if (auto pos = text.find("# Packages"); pos != std::string::npos) {
        text = text.substr(0, pos);
    }

    ui->body->setMarkdown(QString::fromStdString(text));

    // TODO(crueter): Find a way to set default
    const auto assets = release.GetPlatformAssets();

    LOG_INFO(Frontend, "Found {} platform assets", assets.size());
    for (const auto& asset : assets) {
        LOG_INFO(Frontend, "Asset: {} - {}", asset.name, asset.path);
    }

    if (assets.empty()) {
        ui->groupBox->setHidden(true);
        connect(this, &QDialog::accepted, this, [release]() {
            QDesktopServices::openUrl(QUrl{QString::fromStdString(release.html_url)});
        });
    } else if (assets.size() == 1) {
        ui->groupBox->setHidden(true);
        m_asset = assets[0];
        LOG_INFO(Frontend, "Using asset: {}", m_asset.path);
    } else {
        u32 i = 0;
        for (const Common::Net::Asset& a : assets) {
            QRadioButton* r = new QRadioButton(tr(a.name.c_str()), this);
            connect(r, &QRadioButton::toggled, this, [a, this](bool checked) {
                if (checked)
                    m_asset = a;
            });

            if (i == 0)
                r->setChecked(true);
            ++i;

            ui->radioButtons->addWidget(r);
        }
    }
}

UpdateDialog::~UpdateDialog() {
    delete ui;
}

void UpdateDialog::ApplyModernStyle() {
    // Softer, flatter, more "modern" look: rounded surfaces, a single accent
    // color, breathing room, and no jarring native chrome. Applied purely via
    // QSS so it stays a drop-in visual refresh without touching layout logic.
    setMinimumSize(620, 520);

    setStyleSheet(QString::fromUtf8(R"(
        QDialog {
            background-color: #1e1f26;
            color: #e7e7ee;
        }
        QLabel {
            color: #e7e7ee;
            font-size: 13px;
        }
        QLabel#version {
            font-size: 18px;
            font-weight: 600;
            color: #ffffff;
            padding-bottom: 2px;
        }
        QLabel#url {
            color: #8ab4ff;
            font-size: 12px;
        }
        QLabel#label_2 {
            font-size: 13px;
            color: #c4c4d0;
        }
        QTextBrowser#body {
            background-color: #26272f;
            border: 1px solid #34353f;
            border-radius: 10px;
            padding: 10px;
            color: #d6d6e0;
        }
        QGroupBox#groupBox {
            background-color: #26272f;
            border: 1px solid #34353f;
            border-radius: 10px;
            margin-top: 14px;
            padding: 10px 8px 8px 8px;
            font-weight: 600;
            color: #c4c4d0;
        }
        QGroupBox#groupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }
        QRadioButton {
            padding: 4px;
            color: #e7e7ee;
        }
        QProgressBar {
            background-color: #34353f;
            border: none;
            border-radius: 7px;
            height: 14px;
            text-align: center;
            color: #ffffff;
        }
        QProgressBar::chunk {
            background-color: #5b8cff;
            border-radius: 7px;
        }
        QDialogButtonBox QPushButton {
            background-color: #34353f;
            color: #e7e7ee;
            border: none;
            border-radius: 8px;
            padding: 8px 20px;
            font-weight: 600;
            min-width: 84px;
        }
        QDialogButtonBox QPushButton:hover {
            background-color: #3f4150;
        }
        QDialogButtonBox QPushButton:default {
            background-color: #5b8cff;
            color: #ffffff;
        }
        QDialogButtonBox QPushButton:default:hover {
            background-color: #6f99ff;
        }
    )"));

    if (auto* layout = qobject_cast<QGridLayout*>(this->layout())) {
        layout->setContentsMargins(24, 22, 24, 20);
        layout->setSpacing(14);
    }
}

void UpdateDialog::accept() {
    LOG_INFO(Frontend, "accept() called");

    const auto assets = Common::Net::Release(m_release).GetPlatformAssets();

    if (!assets.empty()) {
        Download();
    } else {
        QDialog::accept();
    }
}

void UpdateDialog::Download() {
    LOG_INFO(Frontend, "Download() called");

    // Show the groupBox for progress display
    ui->groupBox->setHidden(false);

    QString downloadUrl;
    if (m_asset.path.empty()) {
        downloadUrl = QString::fromStdString(m_asset.url);
    } else {
        std::string path = m_asset.path;
        if (path.find("http://") == 0 || path.find("https://") == 0) {
            downloadUrl = QString::fromStdString(path);
        } else {
            downloadUrl = QString::fromStdString(m_asset.url + m_asset.path);
        }
    }

    LOG_INFO(Frontend, "Download URL: {}", downloadUrl.toStdString());

    QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                          QLatin1String("/Temp/temp_download_update");
    QDir tempDir(tempDirPath);
    if (!tempDir.exists()) {
        tempDir.mkpath(QLatin1String("."));
    }

    // Work out the *real* extension of the asset we're downloading instead of
    // always assuming .zip -- Linux/FreeBSD assets are .tar.gz, and picking
    // the wrong extension means the platform-specific extraction step below
    // (unzip vs tar) fails outright.
    const QString assetFilename = QString::fromStdString(m_asset.filename);
    m_archiveExtension = QLatin1String(".zip");
    if (assetFilename.endsWith(QLatin1String(".tar.gz"), Qt::CaseInsensitive)) {
        m_archiveExtension = QLatin1String(".tar.gz");
    } else {
        const int dotPos = assetFilename.lastIndexOf(QLatin1Char('.'));
        if (dotPos != -1) {
            m_archiveExtension = assetFilename.mid(dotPos);
        }
    }

    m_downloadPath = tempDirPath + QLatin1String("/temp_download_update") + m_archiveExtension;
    LOG_INFO(Frontend, "Download path: {}", m_downloadPath.toStdString());

    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(true);
    progressBar->setValue(0);

    QLabel* progressLabel = new QLabel(this);
    progressLabel->setText(QString::fromUtf8("0%"));
    progressLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->groupBox->layout());
    if (layout) {
        layout->addWidget(progressBar);
        layout->addWidget(progressLabel);
    }

    QUrl url = QUrl::fromUserInput(downloadUrl);
    QNetworkRequest request(url);
    QNetworkReply* reply = networkManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this,
            [progressBar, progressLabel](qint64 bytesReceived, qint64 bytesTotal) {
                if (bytesTotal > 0) {
                    int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
                    progressBar->setValue(percentage);
                    progressLabel->setText(QString::fromUtf8("%1%").arg(percentage));
                }
            });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, progressBar, progressLabel, tempDirPath]() {
                progressBar->setValue(100);
                progressLabel->setText(QString::fromUtf8("100%"));
                if (reply->error() != QNetworkReply::NoError) {
                    QString baseMsg = QString::fromUtf8(
                        "Network error occurred while trying to download the update:\n");
                    QString errorMsg = baseMsg + reply->errorString();
                    QtCommon::Frontend::Critical(QString::fromUtf8("Download Failed"), errorMsg);
                    reply->deleteLater();
                    progressBar->deleteLater();
                    progressLabel->deleteLater();
                    return;
                }

                QFile file(m_downloadPath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                    reply->deleteLater();

                    auto button = QtCommon::Frontend::Question(
                        QString::fromUtf8("Download Complete"),
                        QString::fromUtf8(
                            "The update has been downloaded. Would you like to install it now?"),
                        QtCommon::Frontend::Yes | QtCommon::Frontend::No);

                    if (button == QtCommon::Frontend::Yes) {
                        Install(tempDirPath);
                    }
                } else {
                    QString baseMsg = QString::fromUtf8("Failed to save the update file at:\n");
                    QString errorMsg = baseMsg + m_downloadPath;
                    QtCommon::Frontend::Critical(QString::fromUtf8("Error"), errorMsg);
                    reply->deleteLater();
                }

                progressBar->deleteLater();
                progressLabel->deleteLater();
            });
}

void UpdateDialog::Install(const QString& tempDirPath) {
    QString rootPath = qApp->applicationDirPath();

#ifdef _WIN32
    QString scriptFileName = tempDirPath + QLatin1String("/update.ps1");
    QString scriptContent = QLatin1String(
        "Set-ExecutionPolicy Bypass -Scope Process -Force\n"
        "Write-Output 'Starting Update...'\n"
        "$tempDir = '%1'\n"
        "$rootDir = '%2'\n"
        "$zipFile = Join-Path $tempDir 'temp_download_update.zip'\n"
        "$extractDir = Join-Path $tempDir 'extracted'\n"
        "\n"
        "Write-Output \"Temp dir: $tempDir\"\n"
        "Write-Output \"Root dir: $rootDir\"\n"
        "Write-Output \"Zip file: $zipFile\"\n"
        "\n"
        "# Wait for the main process to exit\n"
        "$processName = 'eden'\n"
        "$maxWait = 30\n"
        "$waited = 0\n"
        "while (Get-Process -Name $processName -ErrorAction SilentlyContinue) {\n"
        "    Start-Sleep -Seconds 1\n"
        "    $waited++\n"
        "    if ($waited -ge $maxWait) {\n"
        "        Write-Output 'Timeout waiting for process to exit'\n"
        "        break\n"
        "    }\n"
        "}\n"
        "Write-Output 'Process exited, starting update'\n"
        "\n"
        "# Extract zip file\n"
        "Expand-Archive -Path $zipFile -DestinationPath $extractDir -Force\n"
        "Start-Sleep -Seconds 2\n"
        "\n"
        "# Find the inner directory (usually contains the app files)\n"
        "$extractedItems = Get-ChildItem -Path $extractDir\n"
        "Write-Output \"Extracted items: $($extractedItems.Count)\"\n"
        "if ($extractedItems.Count -eq 1 -and $extractedItems[0].PSIsContainer) {\n"
        "    $sourceDir = $extractedItems[0].FullName\n"
        "    Write-Output \"Using source dir: $sourceDir\"\n"
        "} else {\n"
        "    $sourceDir = $extractDir\n"
        "    Write-Output \"Using extract dir as source: $sourceDir\"\n"
        "}\n"
        "\n"
        "# Copy files to root directory, excluding the script itself\n"
        "Get-ChildItem -Path $sourceDir -Recurse | ForEach-Object {\n"
        "    $relativePath = $_.FullName.Substring($sourceDir.Length)\n"
        "    $destPath = Join-Path $rootDir $relativePath.TrimStart('\\')\n"
        "    $destDir = Split-Path $destPath -Parent\n"
        "    if (-not (Test-Path $destDir)) {\n"
        "        New-Item -ItemType Directory -Path $destDir -Force | Out-Null\n"
        "    }\n"
        "    Copy-Item -Path $_.FullName -Destination $destPath -Force\n"
        "    Write-Output \"Copied: $($_.Name) to $destPath\"\n"
        "}\n"
        "\n"
        "Start-Sleep -Seconds 2\n"
        "\n"
        "# Cleanup\n"
        "Remove-Item -Force -LiteralPath (Join-Path $rootDir 'update.ps1') -ErrorAction "
        "SilentlyContinue\n"
        "Remove-Item -Force -LiteralPath $zipFile -ErrorAction SilentlyContinue\n"
        "Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue\n"
        "\n"
        "# Restart application\n"
        "Write-Output 'Restarting application...'\n"
        "Start-Process -FilePath (Join-Path $rootDir 'eden.exe') -WorkingDirectory $rootDir\n"
        "Write-Output 'Update complete'\n");

    QStringList arguments;
    arguments << QLatin1String("-ExecutionPolicy") << QLatin1String("Bypass")
              << QLatin1String("-File") << scriptFileName;
    QString processCommand = QLatin1String("powershell.exe");

    QFile scriptFile(scriptFileName);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        scriptFile.write("\xEF\xBB\xBF");
        QString formattedScript = scriptContent;
        formattedScript.replace(QLatin1String("%1"), tempDirPath);
        formattedScript.replace(QLatin1String("%2"), rootPath);
        out << formattedScript;
        scriptFile.close();

        QProcess::startDetached(processCommand, arguments);

        qApp->quit();
    } else {
        QtCommon::Frontend::Critical(
            QString::fromUtf8("Error"),
            QString::fromUtf8("Failed to create the update script file:\n") + scriptFileName);
    }
#else
    // Linux, macOS, and FreeBSD all install via a generated POSIX shell
    // script, run detached with /bin/sh so it survives us calling qApp->quit().
    // /bin/sh (not bash) is used deliberately: it's guaranteed to exist and
    // be POSIX-compliant on FreeBSD too, where bash is not part of base.
    const QString scriptPath = WriteInstallScript(tempDirPath, rootPath);
    if (scriptPath.isEmpty()) {
        QtCommon::Frontend::Critical(QString::fromUtf8("Error"),
                                     QString::fromUtf8("Failed to create the update script file."));
        return;
    }

    QFile::setPermissions(scriptPath, QFile::permissions(scriptPath) | QFileDevice::ExeOwner |
                                          QFileDevice::ExeGroup | QFileDevice::ExeOther);

    const QString pid = QString::number(qApp->applicationPid());
    QProcess::startDetached(QLatin1String("/bin/sh"), {scriptPath, tempDirPath, rootPath, pid});

    qApp->quit();
#endif
}

#ifndef _WIN32
QString UpdateDialog::WriteInstallScript(const QString& tempDirPath, const QString& rootPath) {
    const QString scriptFileName = tempDirPath + QLatin1String("/update.sh");
    QString scriptContent;

#ifdef __APPLE__
    // On macOS the app ships as a .app bundle and applicationDirPath() points
    // inside it (Contents/MacOS), so we replace the whole bundle two levels
    // up rather than copying loose files into the executable's own folder.
    scriptContent = QLatin1String(
        "#!/bin/sh\n"
        "set -e\n"
        "TEMP_DIR=\"$1\"\n"
        "MACOS_DIR=\"$2\"\n"
        "PID=\"$3\"\n"
        "ARCHIVE=\"$TEMP_DIR/temp_download_update.zip\"\n"
        "EXTRACT_DIR=\"$TEMP_DIR/extracted\"\n"
        "APP_DIR=\"$MACOS_DIR/../..\"\n"
        "APPS_PARENT=\"$(cd \"$APP_DIR/..\" && pwd)\"\n"
        "\n"
        "echo 'Waiting for eden to exit...'\n"
        "i=0\n"
        "while kill -0 \"$PID\" 2>/dev/null; do\n"
        "    sleep 1\n"
        "    i=$((i + 1))\n"
        "    if [ \"$i\" -ge 30 ]; then\n"
        "        echo 'Timeout waiting for process to exit'\n"
        "        break\n"
        "    fi\n"
        "done\n"
        "\n"
        "mkdir -p \"$EXTRACT_DIR\"\n"
        "unzip -o -q \"$ARCHIVE\" -d \"$EXTRACT_DIR\"\n"
        "\n"
        "APP_BUNDLE=$(find \"$EXTRACT_DIR\" -maxdepth 3 -name '*.app' -type d | head -n 1)\n"
        "if [ -z \"$APP_BUNDLE\" ]; then\n"
        "    echo 'Could not locate .app bundle in downloaded update' >&2\n"
        "    exit 1\n"
        "fi\n"
        "\n"
        "BUNDLE_NAME=$(basename \"$APP_BUNDLE\")\n"
        "rm -rf \"$APP_DIR\"\n"
        "cp -R \"$APP_BUNDLE\" \"$APPS_PARENT/$BUNDLE_NAME\"\n"
        "\n"
        "rm -rf \"$TEMP_DIR\"\n"
        "\n"
        "echo 'Restarting application...'\n"
        "open \"$APPS_PARENT/$BUNDLE_NAME\"\n"
        "echo 'Update complete'\n");
#else
    // Linux and FreeBSD both ship as a flat tar.gz of the install directory.
    scriptContent =
        QLatin1String("#!/bin/sh\n"
                      "set -e\n"
                      "TEMP_DIR=\"$1\"\n"
                      "ROOT_DIR=\"$2\"\n"
                      "PID=\"$3\"\n"
                      "ARCHIVE=\"$TEMP_DIR/temp_download_update.tar.gz\"\n"
                      "EXTRACT_DIR=\"$TEMP_DIR/extracted\"\n"
                      "\n"
                      "echo 'Waiting for eden to exit...'\n"
                      "i=0\n"
                      "while kill -0 \"$PID\" 2>/dev/null; do\n"
                      "    sleep 1\n"
                      "    i=$((i + 1))\n"
                      "    if [ \"$i\" -ge 30 ]; then\n"
                      "        echo 'Timeout waiting for process to exit'\n"
                      "        break\n"
                      "    fi\n"
                      "done\n"
                      "\n"
                      "mkdir -p \"$EXTRACT_DIR\"\n"
                      "tar -xzf \"$ARCHIVE\" -C \"$EXTRACT_DIR\"\n"
                      "\n"
                      "# If the archive contains a single top-level directory, install from\n"
                      "# inside it; otherwise treat the extraction root as the source.\n"
                      "ITEM_COUNT=$(find \"$EXTRACT_DIR\" -mindepth 1 -maxdepth 1 | wc -l)\n"
                      "FIRST_ITEM=$(find \"$EXTRACT_DIR\" -mindepth 1 -maxdepth 1 | head -n 1)\n"
                      "if [ \"$ITEM_COUNT\" -eq 1 ] && [ -d \"$FIRST_ITEM\" ]; then\n"
                      "    SRC_DIR=\"$FIRST_ITEM\"\n"
                      "else\n"
                      "    SRC_DIR=\"$EXTRACT_DIR\"\n"
                      "fi\n"
                      "\n"
                      "mkdir -p \"$ROOT_DIR\"\n"
                      "cp -a \"$SRC_DIR/.\" \"$ROOT_DIR/\"\n"
                      "chmod +x \"$ROOT_DIR/eden\" 2>/dev/null || true\n"
                      "\n"
                      "rm -rf \"$TEMP_DIR\"\n"
                      "\n"
                      "echo 'Restarting application...'\n"
                      "\"$ROOT_DIR/eden\" >/dev/null 2>&1 &\n"
                      "echo 'Update complete'\n");
#endif

    QFile scriptFile(scriptFileName);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream out(&scriptFile);
    out << scriptContent;
    scriptFile.close();

    return scriptFileName;
}
#endif // !_WIN32
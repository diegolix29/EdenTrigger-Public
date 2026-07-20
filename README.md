# Eden Experimental Fork

<img width="1024" height="1024" alt="eden trigger" src="https://github.com/user-attachments/assets/ecb1d65d-f0d2-457f-887f-84abf4b1b5e8" />


## Overview

This is an **experimental fork** of the Eden Emulator Project, designed to test and implement various features and optimizations. This fork focuses on enhancing user experience through additional performance monitoring and memory management features.

## 🚀 New Features

### Memory Management
- **Memory Flush Option**: Added a toggle to enable/disable memory flush functionality during gameplay
  - **Location**: Settings → System → App Settings → Enable Memory Flush
  - **Purpose**: Clear app memory and caches for better performance
  - **Usage**: When enabled, shows a memory flush button during gameplay

### Performance Monitoring
- **Phone Temperature Display**: Added real-time phone temperature monitoring
  - **Location**: Settings → Performance Stats → Show Phone Temperature
  - **Purpose**: Monitor device thermal state during emulation
  - **Display**: Shows current phone temperature in the performance overlay

## 📱 Installation

### Installation Steps
1. Download the appropriate APK variant for your device
2. Enable "Install from unknown sources" in Android settings
3. Install the APK
4. Launch the app and follow the setup wizard

## ⚙️ Configuration

### Memory Flush Setup
1. Open **Settings**
2. Navigate to **System** section
3. Find **App Settings** category
4. Toggle **"Enable Memory Flush"**
5. The memory flush button will appear during gameplay

### Temperature Monitoring Setup
1. Open **Settings**
2. Navigate to **Performance Stats** section
3. Toggle **"Show Phone Temperature"**
4. Temperature will display in the performance overlay during gameplay

## 🧪 Experimental Features

This fork includes experimental features that may:
- Enhance performance on certain devices
- Provide additional system monitoring
- Offer new user interaction methods

**Note**: These features are experimental and may not be stable in all configurations.

## 🐛 Bug Reports

Since this is an experimental fork, please report issues with:
1. Device model and Android version
2. APK variant used
3. Steps to reproduce the issue
4. Log files if available

## ⚠️ Disclaimer

This is an **unofficial experimental fork** of Eden Emulator. It is not affiliated with the official Eden Emulator Project. Use at your own risk.

## 📄 License

This project maintains the same license as the original Eden Emulator Project (GPL-3.0-or-later).

## 🔗 Links

- **Original Eden Project**: https://eden-emu.dev
- **Source Code**: https://git.eden-emu.dev/eden-emu
- **Discord**: https://discord.gg/HstXbPch7X


**Thank you for trying this experimental fork!**

## ⚠️ Disclaimer

This is an **unofficial experimental fork** of the Eden Emulator Project.

- I am **not affiliated with, associated with, or endorsed by** the Eden Emulator development team
- This fork is an **independent experiment**
- Any changes, builds, or releases provided here **do not represent the official Eden project**
- I will only update commits and test QoL changes.
  
Use at your own risk.


<img width="1024" height="1024" alt="eden trigger icon" src="https://github.com/user-attachments/assets/c8820d64-a07a-4985-988c-7dfb63dd97e7" />



<!--
# SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
-->
<!-- lang: en-GB -->

<h1 align="center">
  <br>
  <a href="https://git.eden-emu.dev/eden-emu/eden"><img src="./dist/qt_themes/default/icons/256x256/eden.png" alt="Eden" width="200"></a>
  <br>
  <b>Eden</b>
  <br>
</h1>

<h4 align="center"><b>Eden</b> is a free and opensource (FOSS) Switch 1 emulator, derived from Yuzu and Sudachi - started by developer Camille LaVey.
It's written in C++ with portability in mind, with builds for Windows, Linux, macOS, Android, FreeBSD and more.
</h4>

<p align="center">
    </a>
    <a href="https://discord.gg/HstXbPch7X">
        <img src="https://img.shields.io/discord/1367654015269339267?color=5865F2&label=Eden&logo=discord&logoColor=white"
            alt="Discord">
    </a>
    <a href="https://stt.gg/qKgFEAbH">
        <img src="https://img.shields.io/revolt/invite/qKgFEAbH?color=d61f3a&label=Stoat"
            alt="Stoat">
    </a>
</p>

<p align="center">
  <a href="#compatibility">Compatibility</a> |
  <a href="#development">Development</a> |
  <a href="#building">Building</a> |
  <a href="#download">Download</a> |
  <a href="#support">Support</a> |
  <a href="#license">License</a>
</p>

## Compatibility

The emulator is capable of running most commercial games at full speed, provided you meet the necessary hardware requirements.

A list of supported games will be available in future. Please be patient.

Check out our [website](https://eden-emu.dev) for the latest news on exciting features, monthly progress reports, and more!

[![Packaging status](https://repology.org/badge/vertical-allrepos/eden-emulator.svg)](https://repology.org/project/eden-emulator/versions)

## Development

Most of the development happens on our Git server. It is also where [our central repository](https://git.eden-emu.dev/eden-emu/eden) is hosted. For development discussions, please join us on [Discord](https://discord.gg/HstXbPch7X) or [Stoat](https://stt.gg/qKgFEAbH).
You can also follow us on [X (Twitter)](https://nitter.poast.org/edenemuofficial) for updates and announcements.

If you would like to contribute, we are open to new developers and pull requests. Please ensure that your work is of a high standard and properly documented. You can also contact any of the developers on Discord or Stoat to learn more about the current state of the emulator.

See the [sign-up instructions](docs/SIGNUP.md) for information on registration.

Alternatively, if you wish to add translations, go to the [Eden project on Transifex](https://app.transifex.com/edenemu/eden-emulator) and review [the translations README](./dist/languages).

## Documentation

We have a user manual! See our [User Handbook](./docs/user/README.md).

## Building

See the [General Build Guide](docs/Build.md)

For information on provided development tooling, see the [Tools directory](./tools)

## Download

You can download the latest releases from [here](https://git.eden-emu.dev/eden-emu/eden/releases).

Save us some bandwidth! We have [mirrors available](./docs/user/ThirdParty.md#mirrors) as well.

## Support

If you enjoy the project and would like to support us financially, please check out our developers' [donation pages](https://eden-emu.dev/donations)!

Any donations received will go towards things such as:
* Switch consoles to explore and reverse-engineer the hardware
* Switch games for testing, reverse-engineering, and implementing new features
* Web hosting and infrastructure setup
* Additional hardware (e.g. GPUs as needed to improve rendering support, other peripherals to add support for, etc.)
* CI Infrastructure

If you would prefer to support us in a different way, please join our [Discord](https://discord.gg/HstXbPch7X) and talk to Camille or any of our other developers.

## License

Eden is licensed under the GPLv3 (or any later version). Refer to the [LICENSE.txt](https://git.eden-emu.dev/eden-emu/eden/src/branch/master/LICENSE.txt) file.

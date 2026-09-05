// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <thread>
#include <typeinfo>
#include <vector>
#include <QCheckBox>
#include <QComboBox>
#include "common/common_types.h"
#include "common/settings.h"
#include "common/settings_enums.h"
#include "configuration/shared_widget.h"
#include "core/core.h"
#include "ui_configure_cpu.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_cpu.h"

ConfigureCpu::ConfigureCpu(const Core::System& system_,
                           std::shared_ptr<std::vector<ConfigurationShared::Tab*>> group_,
                           const ConfigurationShared::Builder& builder, QWidget* parent)
    : Tab(group_, parent), ui{std::make_unique<Ui::ConfigureCpu>()}, system{system_},
      combobox_translations(builder.ComboboxTranslations()) {
    ui->setupUi(this);

    Setup(builder);

    SetConfiguration();

    connect(accuracy_combobox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureCpu::UpdateGroup);

    connect(backend_combobox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureCpu::UpdateGroup);

    connect(core_config_combobox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureCpu::UpdateCustomCoreUI);

#ifdef HAS_NCE
    ui->backend_group->setVisible(true);
#endif
}

ConfigureCpu::~ConfigureCpu() = default;

void ConfigureCpu::SetConfiguration() {}
void ConfigureCpu::Setup(const ConfigurationShared::Builder& builder) {
    auto* accuracy_layout = ui->widget_accuracy->layout();
    auto* backend_layout = ui->widget_backend->layout();
    auto* core_config_layout = ui->widget_core_config->layout();
    auto* unsafe_layout = ui->unsafe_widget->layout();
    std::map<u32, QWidget*> unsafe_hold{};

    std::vector<Settings::BasicSetting*> settings;
    const auto push = [&](Settings::Category category) {
        for (const auto setting : Settings::values.linkage.by_category[category]) {
            settings.push_back(setting);
        }
    };

    push(Settings::Category::Cpu);
    push(Settings::Category::CpuUnsafe);

    for (const auto setting : settings) {
        auto* widget = builder.BuildWidget(setting, apply_funcs);

        if (widget == nullptr) {
            continue;
        }
        if (!widget->Valid()) {
            widget->deleteLater();
            continue;
        }

        if (setting->Id() == Settings::values.cpu_accuracy.Id()) {
            // Keep track of cpu_accuracy combobox to display/hide the unsafe settings
            accuracy_layout->addWidget(widget);
            accuracy_combobox = widget->combobox;
        } else if (setting->Id() == Settings::values.cpu_backend.Id()) {
            backend_layout->addWidget(widget);
            backend_combobox = widget->combobox;
        } else if (setting->Id() == Settings::values.cpu_ticks.Id()) {
        } else if (setting->Id() == Settings::values.cpu_core_config.Id()) {
            core_config_layout->addWidget(widget);
            core_config_combobox = widget->combobox;
        } else if (setting->Id() == Settings::values.fast_cpu_time.Id() ||
                   setting->Id() == Settings::values.vtable_bouncing.Id() ||
                   setting->Id() == Settings::values.cpu_ticks.Id()) {
        } else if (setting->Id() == Settings::values.fast_cpu_time.Id() ||
                   setting->Id() == Settings::values.cpu_ticks.Id()) {
            ui->general_layout->addWidget(widget);
        } else {
            // Presently, all other settings here are unsafe checkboxes
            unsafe_hold.insert({setting->Id(), widget});
        }
    }

    for (const auto& [label, widget] : unsafe_hold) {
        unsafe_layout->addWidget(widget);
    }

    UpdateGroup();
}

void ConfigureCpu::UpdateGroup() {
    const u32 accuracy = accuracy_combobox->currentIndex();
    const u32 backend = backend_combobox->currentIndex();
    // TODO(crueter): see if this works on NCE
    ui->unsafe_group->setVisible(accuracy == (u32)Settings::CpuAccuracy::Unsafe &&
                                 backend == (u32)Settings::CpuBackend::Dynarmic);
}

void ConfigureCpu::UpdateCustomCoreUI() {
    const u32 core_config = core_config_combobox->currentIndex();
    const bool is_custom = core_config == (u32)Settings::CpuCoreConfig::Custom;

    // Show/hide the custom core selection widget
    ui->custom_core_group->setVisible(is_custom);

    if (is_custom) {
        // Initialize custom core checkboxes if not already done
        if (custom_core_checkboxes.empty()) {
            SetupCustomCoreCheckboxes();
        }
    }
}

void ConfigureCpu::SetupCustomCoreCheckboxes() {
    const int num_cores = std::thread::hardware_concurrency();
    const u64 current_custom_cores = Settings::values.cpu_custom_cores.GetValue();

    custom_core_checkboxes.clear();

    for (int i = 0; i < num_cores; ++i) {
        auto* checkbox = new QCheckBox(tr("Core %1").arg(i), this);
        checkbox->setChecked((current_custom_cores & (1ULL << i)) != 0);
        ui->custom_core_layout->addWidget(checkbox);
        custom_core_checkboxes.append(checkbox);
    }
}

void ConfigureCpu::ApplyConfiguration() {
    const bool is_powered_on = system.IsPoweredOn();

    // Save custom core selection if custom mode is enabled
    if (core_config_combobox->currentIndex() == (u32)Settings::CpuCoreConfig::Custom) {
        u64 custom_cores = 0;
        for (int i = 0; i < custom_core_checkboxes.size(); ++i) {
            if (custom_core_checkboxes[i]->isChecked()) {
                custom_cores |= (1ULL << i);
            }
        }
        Settings::values.cpu_custom_cores.SetValue(custom_cores);
    }

    for (const auto& apply_func : apply_funcs) {
        apply_func(is_powered_on);
    }
}

void ConfigureCpu::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureCpu::RetranslateUI() {
    ui->retranslateUi(this);
}

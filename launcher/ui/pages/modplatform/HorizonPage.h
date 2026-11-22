// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <QWidget>

#include "BaseVersion.h"
#include "minecraft/HorizonVersion.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class HorizonPage;
}

class NewInstanceDialog;

class HorizonPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit HorizonPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~HorizonPage() override;

    QString displayName() const override { return tr("Horizon"); }
    QIcon icon() const override { return QIcon::fromTheme("minecraft"); }
    QString id() const override { return "horizon"; }
    void retranslate() override;

    void openedImpl() override;

   private slots:
    void filterChanged();
    void setSelectedVersion(BaseVersion::Ptr version);
    void suggestCurrent();

   private:
    NewInstanceDialog* m_dialog = nullptr;
    Ui::HorizonPage* m_ui = nullptr;
    HorizonVersionList* m_versionList = nullptr;
    BaseVersion::Ptr m_selectedVersion;
    bool m_initialized = false;
};

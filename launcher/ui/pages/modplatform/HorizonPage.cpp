// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "HorizonPage.h"
#include "ui_HorizonPage.h"

#include <QCheckBox>
#include <QRegularExpression>
#include <utility>

#include "Filter.h"
#include "minecraft/HorizonCreationTask.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/widgets/VersionSelectWidget.h"

HorizonPage::HorizonPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog), m_ui(new Ui::HorizonPage)
{
    m_ui->setupUi(this);
    m_versionList = new HorizonVersionList(this);

    connect(m_ui->versionList, &VersionSelectWidget::selectedVersionChanged, this, &HorizonPage::setSelectedVersion);
    connect(m_ui->nightlyFilter, &QCheckBox::checkStateChanged, this, &HorizonPage::filterChanged);
    connect(m_ui->snapshotFilter, &QCheckBox::checkStateChanged, this, &HorizonPage::filterChanged);
    connect(m_ui->refreshBtn, &QPushButton::clicked, this, [this] { m_ui->versionList->loadList(true); });
}

HorizonPage::~HorizonPage()
{
    delete m_ui;
}

void HorizonPage::openedImpl()
{
    if (!m_initialized) {
        m_ui->versionList->initialize(m_versionList);
        m_initialized = true;
    } else {
        suggestCurrent();
    }
    filterChanged();
}

void HorizonPage::retranslate()
{
    m_ui->retranslateUi(this);
}

void HorizonPage::filterChanged()
{
    QStringList channels;
    if (m_ui->nightlyFilter->isChecked())
        channels << "(nightly)";
    if (m_ui->snapshotFilter->isChecked())
        channels << "(snapshot)";
    m_ui->versionList->setFilter(BaseVersionList::TypeRole, Filters::regexp(QRegularExpression(channels.join('|'))));
}

void HorizonPage::suggestCurrent()
{
    if (!isOpened)
        return;

    if (!m_selectedVersion) {
        m_dialog->setSuggestedPack();
        return;
    }

    auto version = std::static_pointer_cast<HorizonVersion>(m_selectedVersion);
    m_dialog->setSuggestedPack(version->releaseName, new HorizonCreationTask(version));
    m_dialog->setSuggestedIcon("default");
}

void HorizonPage::setSelectedVersion(BaseVersion::Ptr version)
{
    m_selectedVersion = std::move(version);
    suggestCurrent();
}

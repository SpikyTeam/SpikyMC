// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <QDateTime>
#include <QHash>

#include "BaseVersion.h"
#include "BaseVersionList.h"

class HorizonVersion : public BaseVersion {
   public:
    using Ptr = std::shared_ptr<HorizonVersion>;

    QString tag;
    QString releaseName;
    QString channel;
    QString archiveUrl;
    QString archiveSha256;
    QDateTime published;
    bool recommended = false;

    QString descriptor() const override { return tag; }
    QString name() const override { return releaseName; }
    QString typeString() const override { return channel; }
};

class HorizonVersionList : public BaseVersionList {
    Q_OBJECT
   public:
    explicit HorizonVersionList(QObject* parent = nullptr);

    Task::Ptr getLoadTask(bool forceReload = false) override;
    bool isLoaded() override { return m_loaded; }
    const BaseVersion::Ptr at(int i) const override { return m_versions.at(i); }
    int count() const override { return m_versions.size(); }
    void sortVersions() override;

    QVariant data(const QModelIndex& index, int role) const override;
    RoleList providesRoles() const override;

   protected slots:
    void updateListData(QList<BaseVersion::Ptr>) override {}

   private:
    void parse(const QByteArray& data);

    QList<BaseVersion::Ptr> m_versions;
    bool m_loaded = false;
};

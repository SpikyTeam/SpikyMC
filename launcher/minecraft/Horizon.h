// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <QString>
#include <QStringList>
#include <optional>

namespace Horizon {
//! Files pinned by research-lock.json that the launcher has to provide.
struct Lock {
    QString minecraftVersion;
    int javaMajor = 0;
    QString clientSha1;
    QString serverSha1;
    //! SHA-1 of the Mojang version JSON, plus older republished ones Horizon still accepts.
    QString manifestSha1;
    QStringList acceptedManifestSha1;
};

//! Release archive for the platform this build of the launcher runs on, or empty if unsupported.
QString platformAssetName();

//! Absolute path to the build folder, from an instance-relative path stored in settings.
QString buildDir(const QString& instanceRoot, const QString& relativeBuildDir);

//! Finds the folder holding research-lock.json below the extraction root.
QString findBuildDir(const QString& root);

std::optional<Lock> readLock(const QString& buildDir);

QString binary(const QString& buildDir, const QString& name);
QString serverJarPath(const QString& instanceRoot);

//! Prepared resources are shared between instances of the same build.
QString resourcesDir(const QString& versionTag);

//! JDK/Mojang runtime home derived from a java executable path, empty if it can't be derived.
QString javaHome(const QString& javaPath);
}  // namespace Horizon

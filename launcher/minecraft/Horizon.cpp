// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "Horizon.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>

#include "FileSystem.h"

namespace Horizon {
QString platformAssetName()
{
    const auto arch = QSysInfo::buildCpuArchitecture();
#if defined(Q_OS_WIN)
    if (arch == "arm64")
        return "horizon-windows-arm64.zip";
    if (arch == "x86_64")
        return "horizon-windows-x64.zip";
#elif defined(Q_OS_MACOS)
    if (arch == "arm64")
        return "horizon-macos-arm64.zip";
    if (arch == "x86_64")
        return "horizon-macos-x64.zip";
#else
    if (arch == "x86_64")
        return "horizon-linux-x86_64.tar.gz";
#endif
    return {};
}

QString buildDir(const QString& instanceRoot, const QString& relativeBuildDir)
{
    if (relativeBuildDir.isEmpty())
        return {};
    return FS::PathCombine(instanceRoot, relativeBuildDir);
}

QString findBuildDir(const QString& root)
{
    QDir dir(root);
    if (QFileInfo::exists(dir.filePath("research-lock.json")))
        return dir.absolutePath();
    for (const auto& entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        auto candidate = dir.filePath(entry);
        if (QFileInfo::exists(FS::PathCombine(candidate, "research-lock.json")))
            return candidate;
    }
    return {};
}

std::optional<Lock> readLock(const QString& buildDir)
{
    QFile file(FS::PathCombine(buildDir, "research-lock.json"));
    if (!file.open(QFile::ReadOnly))
        return std::nullopt;
    auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return std::nullopt;
    auto minecraft = doc.object().value("minecraft").toObject();
    if (minecraft.isEmpty())
        return std::nullopt;

    Lock lock;
    lock.minecraftVersion = minecraft.value("version").toString();
    lock.javaMajor = minecraft.value("java_major").toInt();
    lock.clientSha1 = minecraft.value("client_sha1").toString();
    lock.serverSha1 = minecraft.value("server_sha1").toString();
    lock.manifestSha1 = minecraft.value("manifest_sha1").toString();
    for (const auto& sha1 : minecraft.value("accepted_manifest_sha1").toArray())
        lock.acceptedManifestSha1.append(sha1.toString());
    if (lock.minecraftVersion.isEmpty() || lock.serverSha1.isEmpty())
        return std::nullopt;
    return lock;
}

QString binary(const QString& buildDir, const QString& name)
{
#ifdef Q_OS_WIN
    return FS::PathCombine(buildDir, name + ".exe");
#else
    return FS::PathCombine(buildDir, name);
#endif
}

QString serverJarPath(const QString& instanceRoot)
{
    return FS::PathCombine(instanceRoot, "horizon", "server.jar");
}

QString resourcesDir(const QString& versionTag)
{
    return FS::PathCombine(QDir("horizon").absolutePath(), FS::RemoveInvalidPathChars(versionTag));
}

QString javaHome(const QString& javaPath)
{
    auto resolved = FS::ResolveExecutable(javaPath);
    if (resolved.isEmpty())
        return {};
    QDir binDir = QFileInfo(resolved).absoluteDir();
    if (binDir.dirName().compare("bin", Qt::CaseInsensitive) != 0)
        return {};
    if (!binDir.cdUp())
        return {};
    return binDir.absolutePath();
}
}  // namespace Horizon

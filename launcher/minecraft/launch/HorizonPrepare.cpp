// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "HorizonPrepare.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "FileSystem.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "net/ApiRequest.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"

namespace {
QString sha1File(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(&file))
        return {};
    return QString::fromLatin1(hash.result().toHex());
}

bool matchesSha1(const QString& path, const QString& expected)
{
    return !expected.isEmpty() && sha1File(path) == expected;
}

QString launcherFile(const QString& relative)
{
    return QDir::current().absoluteFilePath(relative);
}
}  // namespace

HorizonPrepare::HorizonPrepare(LaunchTask* parent) : LaunchStep(parent), m_process(QStringConverter::Utf8)
{
    connect(&m_process, &LoggedProcess::log, this, &HorizonPrepare::onLog);
    connect(&m_process, &LoggedProcess::stateChanged, this, &HorizonPrepare::onState);
}

void HorizonPrepare::executeTask()
{
    auto* instance = m_parent->instance();
    m_buildDir = instance->horizonBuildDir();
    if (m_buildDir.isEmpty() || !QFileInfo::exists(Horizon::binary(m_buildDir, "horizon-prepare"))) {
        emitFailed(tr("The Horizon build is missing. Reinstall the instance."));
        return;
    }

    auto lock = Horizon::readLock(m_buildDir);
    if (!lock) {
        emitFailed(tr("Could not read research-lock.json from the Horizon build."));
        return;
    }
    m_lock = *lock;

    m_resourcesDir = instance->horizonResourcesDir();
    m_serverJar = Horizon::serverJarPath(instance->instanceRoot());
    m_versionJson = FS::PathCombine(QFileInfo(m_serverJar).absolutePath(), m_lock.minecraftVersion + ".json");
    m_process.setWorkingDirectory(m_buildDir);
    runCheck();
}

void HorizonPrepare::runCheck()
{
    runProcess(Phase::Check, { "--check", "--output", m_resourcesDir });
}

QStringList HorizonPrepare::prepareArgs() const
{
    auto version = m_lock.minecraftVersion;
    auto args = QStringList{ "--prepare",
                             "--server-jar",
                             m_serverJar,
                             "--version-json",
                             m_versionJson,
                             "--client-jar",
                             launcherFile("libraries/com/mojang/minecraft/" + version + "/minecraft-" + version + "-client.jar"),
                             "--libraries-dir",
                             launcherFile("libraries"),
                             "--assets-dir",
                             launcherFile("assets"),
                             "--output",
                             m_resourcesDir,
                             "--complete",
                             "--progress",
                             "json" };

    auto javaHome = Horizon::javaHome(m_parent->instance()->settings()->get("JavaPath").toString());
    if (!javaHome.isEmpty())
        args << "--java-home" << javaHome;
    return args;
}

void HorizonPrepare::runProcess(Phase phase, const QStringList& args)
{
    m_phase = phase;
    setStatus(phase == Phase::Check ? tr("Checking Horizon resources") : tr("Preparing Horizon resources"));
    m_process.start(Horizon::binary(m_buildDir, "horizon-prepare"), args);
}

void HorizonPrepare::ensureVersionJson()
{
    // Horizon pins the exact bytes of Mojang's version JSON, so the launcher's rewritten copy won't do.
    m_versionJsonSha1.clear();
    for (const auto& sha1 : m_lock.acceptedManifestSha1) {
        if (!sha1.isEmpty() && !m_versionJsonSha1.contains(sha1))
            m_versionJsonSha1.append(sha1);
    }
    if (!m_lock.manifestSha1.isEmpty() && !m_versionJsonSha1.contains(m_lock.manifestSha1))
        m_versionJsonSha1.prepend(m_lock.manifestSha1);

    if (m_versionJsonSha1.contains(sha1File(m_versionJson))) {
        ensureServerJar();
        return;
    }
    m_versionJsonIndex = 0;
    downloadVersionJson();
}

void HorizonPrepare::downloadVersionJson()
{
    if (m_versionJsonIndex >= m_versionJsonSha1.size()) {
        emitFailed(tr("Could not download the Minecraft %1 version JSON required by Horizon.").arg(m_lock.minecraftVersion));
        return;
    }
    const auto sha1 = m_versionJsonSha1.at(m_versionJsonIndex++);
    setStatus(tr("Downloading the Minecraft version JSON"));
    FS::ensureFolderPathExists(QFileInfo(m_versionJson).absolutePath());

    // piston-meta paths are content-addressed, so the lock's hashes are the download URLs.
    auto url = QUrl("https://piston-meta.mojang.com/v1/packages/" + sha1 + "/" + m_lock.minecraftVersion + ".json");
    auto job = makeShared<NetJob>(tr("Horizon version JSON"), APPLICATION->network());
    auto request = Net::ApiRequest::makeFile(url, m_versionJson);
    request->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, sha1));
    job->addNetAction(request);

    connect(job.get(), &NetJob::succeeded, this, [this] { ensureServerJar(); });
    connect(job.get(), &NetJob::failed, this, [this](const QString&) {
        QFile::remove(m_versionJson);
        downloadVersionJson();
    });
    connect(job.get(), &NetJob::aborted, this, [this] { emitAborted(); });
    m_task = job;
    job->start();
}

void HorizonPrepare::ensureServerJar()
{
    if (matchesSha1(m_serverJar, m_lock.serverSha1)) {
        runProcess(Phase::Prepare, prepareArgs());
        return;
    }
    downloadServerJar();
}

void HorizonPrepare::downloadServerJar()
{
    setStatus(tr("Downloading the Minecraft server jar"));
    FS::ensureFolderPathExists(QFileInfo(m_serverJar).absolutePath());

    auto url = QUrl("https://piston-data.mojang.com/v1/objects/" + m_lock.serverSha1 + "/server.jar");
    auto job = makeShared<NetJob>(tr("Horizon server jar"), APPLICATION->network());
    auto request = Net::ApiRequest::makeFile(url, m_serverJar);
    request->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, m_lock.serverSha1));
    job->addNetAction(request);

    connect(job.get(), &NetJob::succeeded, this, [this] { runProcess(Phase::Prepare, prepareArgs()); });
    connect(job.get(), &NetJob::failed, this, [this](const QString& reason) {
        emitFailed(tr("Failed to download the Minecraft server jar:\n%1").arg(reason));
    });
    connect(job.get(), &NetJob::aborted, this, [this] { emitAborted(); });
    m_task = job;
    job->start();
}

void HorizonPrepare::onLog(const QStringList& lines, MessageLevel level)
{
    if (m_phase != Phase::Prepare) {
        emit logLines(lines, level);
        return;
    }

    for (const auto& line : lines) {
        auto object = QJsonDocument::fromJson(line.toUtf8()).object();
        if (object.contains("fraction")) {
            setProgress(static_cast<qint64>(object.value("fraction").toDouble() * 1000), 1000);
        } else if (object.contains("message")) {
            emit logLine(object.value("message").toString(), level);
        } else if (object.contains("error")) {
            emit logLine(object.value("error").toString(), MessageLevel::Error);
        } else {
            emit logLine(line, level);
        }
    }
}

void HorizonPrepare::onState(LoggedProcess::State state)
{
    switch (state) {
        case LoggedProcess::FailedToStart:
            emitFailed(tr("Could not start horizon-prepare: %1").arg(m_process.errorString()));
            return;
        case LoggedProcess::Aborted:
            emitAborted();
            return;
        case LoggedProcess::Crashed:
            emitFailed(tr("horizon-prepare crashed."));
            return;
        case LoggedProcess::Finished: {
            auto code = m_process.exitCode();
            if (m_phase == Phase::Check) {
                if (code == 0)
                    emitSucceeded();
                else
                    ensureVersionJson();
                return;
            }
            if (code == 0)
                emitSucceeded();
            else
                emitFailed(tr("Preparing Horizon resources failed (exit code %1).").arg(code));
            return;
        }
        default:
            return;
    }
}

bool HorizonPrepare::abort()
{
    if (m_task && m_task->isRunning())
        return m_task->abort();
    if (m_process.state() == LoggedProcess::Running || m_process.state() == LoggedProcess::Starting)
        m_process.kill();
    return true;
}

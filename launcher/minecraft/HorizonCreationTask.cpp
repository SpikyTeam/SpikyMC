// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "HorizonCreationTask.h"

#include <QDir>
#include <QProcess>
#include <utility>

#include "Application.h"
#include "FileSystem.h"
#include "Horizon.h"
#include "MMCZip.h"
#include "archive/ExtractZipTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/ApiRequest.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"
#include "settings/INISettingsObject.h"

HorizonCreationTask::HorizonCreationTask(HorizonVersion::Ptr version) : m_version(std::move(version)) {}

void HorizonCreationTask::executeTask()
{
    setStatus(tr("Creating Horizon instance %1").arg(m_version->releaseName));

    m_instance = std::make_unique<MinecraftInstance>(
        m_globalSettings, std::make_unique<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg")), m_stagingPath);
    m_instance->settings()->set("HorizonVersion", m_version->tag);
    m_instance->setName(name());
    m_instance->setIconKey(m_instIcon);
    downloadArchive();
}

void HorizonCreationTask::downloadArchive()
{
    m_archivePath = FS::PathCombine(m_stagingPath, "horizon-download");
    auto job = makeShared<NetJob>(tr("Download Horizon build"), APPLICATION->network());
    auto request = Net::ApiRequest::makeFile(QUrl(m_version->archiveUrl), m_archivePath);
    request->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, m_version->archiveSha256));
    job->addNetAction(request);

    connect(job.get(), &NetJob::succeeded, this, &HorizonCreationTask::extractArchive);
    connect(job.get(), &NetJob::failed, this, [this](const QString& reason) {
        emitFailed(tr("Failed to download the Horizon build:\n%1").arg(reason));
    });
    connect(job.get(), &NetJob::aborted, this, [this] { emitAborted(); });
    m_task = job;
    job->start();
}

void HorizonCreationTask::extractArchive()
{
    setStatus(tr("Extracting Horizon build"));
    auto root = FS::PathCombine(m_stagingPath, "horizon");
    if (!FS::ensureFolderPathExists(root)) {
        emitFailed(tr("Failed to create the Horizon folder."));
        return;
    }
    auto task = makeShared<MMCZip::ExtractZipTask>(m_archivePath, QDir(root));
    connect(task.get(), &Task::succeeded, this, &HorizonCreationTask::finishSetup);
    connect(task.get(), &Task::failed, this, [this](const QString& reason) {
        emitFailed(tr("Failed to extract the Horizon build:\n%1").arg(reason));
    });
    connect(task.get(), &Task::aborted, this, [this] { emitAborted(); });
    m_task = task;
    task->start();
}

void HorizonCreationTask::finishSetup()
{
    FS::deletePath(m_archivePath);

    auto buildDir = Horizon::findBuildDir(FS::PathCombine(m_stagingPath, "horizon"));
    if (buildDir.isEmpty()) {
        emitFailed(tr("The Horizon build is missing research-lock.json."));
        return;
    }
    auto lock = Horizon::readLock(buildDir);
    if (!lock) {
        emitFailed(tr("Could not read the Horizon build's research-lock.json."));
        return;
    }

#ifdef Q_OS_MACOS
    // The build is signed ad hoc; Gatekeeper refuses it while it carries the download quarantine.
    QProcess::execute("/usr/bin/xattr", { "-dr", "com.apple.quarantine", buildDir });
#endif

    auto* settings = m_instance->settings();
    settings->set("HorizonBuildDir", QDir(m_stagingPath).relativeFilePath(buildDir));
    settings->set("HorizonResourcesDir", Horizon::resourcesDir(m_version->tag));

    {
        const SettingsObject::Lock settingsLock(settings);
        auto* components = m_instance->getPackProfile();
        components->buildingFromScratch();
        components->setComponentVersion("net.minecraft", lock->minecraftVersion, true);
        components->saveNow();
    }

    downloadFiles(m_instance.get());
}

bool HorizonCreationTask::abort()
{
    if (m_task && m_task->isRunning())
        return m_task->abort();
    return InstanceTask::abort();
}

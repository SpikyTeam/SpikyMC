// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "HorizonClientLaunch.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "launch/LaunchTask.h"
#include "minecraft/Horizon.h"
#include "minecraft/MinecraftInstance.h"

HorizonClientLaunch::HorizonClientLaunch(LaunchTask* parent) : LaunchStep(parent), m_process(QStringConverter::Utf8)
{
    connect(&m_process, &LoggedProcess::log, this, &HorizonClientLaunch::logLines);
    connect(&m_process, &LoggedProcess::log, this, [this](const QStringList& lines, MessageLevel level) {
        if (level != MessageLevel::StdErr)
            return;
        for (const auto& line : lines) {
            if (!line.trimmed().isEmpty())
                m_lastError = line.trimmed();
        }
    });
    connect(&m_process, &LoggedProcess::stateChanged, this, &HorizonClientLaunch::onState);
}

void HorizonClientLaunch::executeTask()
{
    auto* instance = m_parent->instance();
    auto* settings = instance->settings();
    auto buildDir = instance->horizonBuildDir();
    auto client = Horizon::binary(buildDir, "horizon-client");
    if (!QFileInfo::exists(client)) {
        const char* reason = QT_TR_NOOP("The Horizon client is missing. Reinstall the instance.");
        emit logLine(tr(reason), MessageLevel::Fatal);
        emitFailed(tr(reason));
        return;
    }

    auto resources = instance->horizonResourcesDir();

    auto gameDir = instance->gameRoot();
    auto args = QStringList{ "--resources", resources, "--game-dir", gameDir, "--home", gameDir, "--session-stdin" };

    auto width = settings->get("MinecraftWinWidth").toInt();
    auto height = settings->get("MinecraftWinHeight").toInt();
    if (width > 0 && height > 0)
        args << "--width" << QString::number(width) << "--height" << QString::number(height);
    if (settings->get("LaunchMaximized").toBool())
        args << "--fullscreen";

    if (m_targetToJoin) {
        if (!m_targetToJoin->world.isEmpty()) {
            args << "--quickPlaySingleplayer" << m_targetToJoin->world;
        } else if (!m_targetToJoin->address.isEmpty()) {
            auto address = m_targetToJoin->address;
            if (m_targetToJoin->port != 25565)
                address = QString("%1:%2").arg(address).arg(m_targetToJoin->port);
            args << "--quickPlayMultiplayer" << address;
        }
    }

    m_process.setWorkingDirectory(gameDir);
    // Keep the game running if the launcher closes.
    m_process.setDetachable(true);
    m_process.start(client, args);
}

QByteArray HorizonClientLaunch::sessionJson() const
{
    QJsonObject session;
    if (m_session) {
        session.insert("name", m_session->player_name);
        session.insert("uuid", m_session->uuid);
        session.insert("access_token", m_session->access_token);
        if (m_session->launchMode != LaunchMode::Offline)
            session.insert("user_type", "msa");
    }
    return QJsonDocument(session).toJson(QJsonDocument::Compact);
}

void HorizonClientLaunch::onState(LoggedProcess::State state)
{
    switch (state) {
        case LoggedProcess::FailedToStart:
            emitFailed(tr("Could not launch Horizon: %1").arg(m_process.errorString()));
            return;
        case LoggedProcess::Aborted:
            m_parent->setPid(-1);
            m_parent->instance()->setMinecraftRunning(false);
            emitAborted();
            return;
        case LoggedProcess::Crashed:
            m_parent->setPid(-1);
            m_parent->instance()->setMinecraftRunning(false);
            emitFailed(tr("Game crashed."));
            return;
        case LoggedProcess::Finished: {
            auto* instance = m_parent->instance();
            if (instance->settings()->get("CloseAfterLaunch").toBool())
                APPLICATION->showMainWindow();

            m_parent->setPid(-1);
            instance->setMinecraftRunning(false);
            auto exitCode = m_process.exitCode();
            if (exitCode != 0) {
                // Horizon's last stderr line names the crash report.
                auto reason = tr("Game crashed (exit code %1).").arg(exitCode);
                if (!m_lastError.isEmpty())
                    reason += "\n" + m_lastError;
                emitFailed(reason);
                return;
            }
            emitSucceeded();
            return;
        }
        case LoggedProcess::Running:
            emit logLine(QString("Horizon process ID: %1\n\n").arg(m_process.processId()), MessageLevel::Launcher);
            m_parent->setPid(m_process.processId());

            // The game reads the session from stdin, so this also holds it back until we proceed.
            m_mayProceed = true;
            emit readyForLaunch();
            return;
        default:
            return;
    }
}

void HorizonClientLaunch::proceed()
{
    if (!m_mayProceed)
        return;
    m_mayProceed = false;
    m_parent->instance()->setMinecraftRunning(true);
    m_process.write(sessionJson());
    m_process.closeWriteChannel();
    if (m_parent->instance()->settings()->get("CloseAfterLaunch").toBool())
        APPLICATION->closeAllWindows();
}

bool HorizonClientLaunch::abort()
{
    m_mayProceed = false;
    auto state = m_process.state();
    if (state == LoggedProcess::Running || state == LoggedProcess::Starting)
        m_process.kill();
    return true;
}

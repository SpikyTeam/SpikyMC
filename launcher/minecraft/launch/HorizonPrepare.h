// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <launch/LaunchStep.h>
#include <LoggedProcess.h>

#include "minecraft/Horizon.h"

class HorizonPrepare : public LaunchStep {
    Q_OBJECT
   public:
    explicit HorizonPrepare(LaunchTask* parent);
    ~HorizonPrepare() override = default;

    void executeTask() override;
    bool abort() override;
    bool canAbort() const override { return true; }

   private slots:
    void onLog(const QStringList& lines, MessageLevel level);
    void onState(LoggedProcess::State state);

   private:
    enum class Phase { Check, Prepare };

    void runCheck();
    void runProcess(Phase phase, const QStringList& args);
    void ensureVersionJson();
    void downloadVersionJson();
    void ensureServerJar();
    void downloadServerJar();
    QStringList prepareArgs() const;

    LoggedProcess m_process;
    Task::Ptr m_task;
    Phase m_phase = Phase::Check;
    Horizon::Lock m_lock;
    QString m_buildDir;
    QString m_resourcesDir;
    QString m_versionJson;
    QStringList m_versionJsonSha1;
    int m_versionJsonIndex = 0;
    QString m_serverJar;
};

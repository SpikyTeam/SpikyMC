// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <LoggedProcess.h>
#include <launch/LaunchStep.h>
#include <minecraft/auth/AuthSession.h>

#include "minecraft/launch/MinecraftTarget.h"

class HorizonClientLaunch : public LaunchStep {
    Q_OBJECT
   public:
    explicit HorizonClientLaunch(LaunchTask* parent);
    ~HorizonClientLaunch() override = default;

    void executeTask() override;
    bool abort() override;
    void proceed() override;
    bool canAbort() const override { return true; }

    void setAuthSession(AuthSessionPtr session) { m_session = session; }
    void setTargetToJoin(MinecraftTarget::Ptr targetToJoin) { m_targetToJoin = std::move(targetToJoin); }

   private slots:
    void onState(LoggedProcess::State state);

   private:
    QByteArray sessionJson() const;

    LoggedProcess m_process;
    AuthSessionPtr m_session;
    MinecraftTarget::Ptr m_targetToJoin;
    QString m_lastError;
    bool m_mayProceed = false;
};

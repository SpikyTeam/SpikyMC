// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <memory>

#include "HorizonVersion.h"
#include "InstanceTask.h"

class MinecraftInstance;

class HorizonCreationTask final : public InstanceTask {
    Q_OBJECT
   public:
    explicit HorizonCreationTask(HorizonVersion::Ptr version);

    void executeTask() override;
    bool abort() override;

   private:
    void downloadArchive();
    void extractArchive();
    void finishSetup();

    std::unique_ptr<MinecraftInstance> m_instance;
    HorizonVersion::Ptr m_version;
    Task::Ptr m_task;
    QString m_archivePath;
};

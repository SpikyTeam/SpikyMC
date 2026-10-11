// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *  Copyright (c) 2025 Octol1ttle <l1ttleofficial@outlook.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright (c) 2025 Octol1ttle <l1ttleofficial@outlook.com>
 *
 *      Licensed under the GNU General Public License, version 3.
 */

#pragma once
#include "MSAStep.h"

class ElyStep : public MSAStep {
    Q_OBJECT

   public:
    explicit ElyStep(AccountData* data, bool silent = false);
    virtual ~ElyStep() noexcept = default;

    QString describe() override;
};
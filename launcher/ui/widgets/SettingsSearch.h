// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  Search index for the settings dialog.
 *  Matches setting ID, localized text and English source text at once.
 */

#pragma once

#include <QList>
#include <QString>

class BasePage;
class QWidget;

class SettingsSearch {
   public:
    struct Entry {
        BasePage* page = nullptr;
        QWidget* target = nullptr;
        QString title;
        QString english;
        QString id;
    };

    void build(const QList<BasePage*>& pages);
    QList<Entry> query(const QString& text, int limit = 50) const;
    bool isEmpty() const { return m_entries.isEmpty(); }

   private:
    QList<Entry> m_entries;
};

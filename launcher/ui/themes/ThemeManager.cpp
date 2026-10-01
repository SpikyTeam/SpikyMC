// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 */
#include "ThemeManager.h"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QIcon>
#include <QImageReader>
#include "Result.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/SpikyMCTheme.h"

#include "Application.h"
#include "settings/SettingsObject.h"

ThemeManager::ThemeManager()
{
    QIcon::setFallbackThemeName(QIcon::themeName());
    QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << m_iconThemeFolder.path());

    initializeThemes();
}

ThemeManager::~ThemeManager()
{
    stopSettingNewWindowColorsOnMac();
}

/// @brief Adds the Theme to the list of themes
/// @param theme The Theme to add
/// @return Theme ID
QString ThemeManager::addTheme(std::unique_ptr<ITheme> theme)
{
    QString id = theme->id();
    if (m_themes.find(id) == m_themes.end())
        m_themes.emplace(id, std::move(theme));
    else
        themeWarningLog() << "Theme(" << id << ") not added to prevent id duplication";
    return id;
}

/// @brief Gets the Theme from the List via ID
/// @param themeId Theme ID of theme to fetch
/// @return Theme at themeId
ITheme* ThemeManager::getTheme(QString themeId)
{
    return m_themes[themeId].get();
}

QString ThemeManager::addIconTheme(IconTheme theme)
{
    QString id = theme.id();
    if (m_icons.find(id) == m_icons.end())
        m_icons.emplace(id, std::move(theme));
    else
        themeWarningLog() << "IconTheme(" << id << ") not added to prevent id duplication";
    return id;
}

void ThemeManager::initializeThemes()
{
    // Icon themes
    initializeIcons();

    // Initialize widget themes
    initializeWidgets();
}

void ThemeManager::initializeIcons()
{
    // TODO: icon themes and instance icons do not mesh well together. Rearrange and fix discrepancies!
    // set icon theme search path!
    themeDebugLog() << "<> Initializing Icon Themes";

    for (const QString& id : builtinIcons) {
        IconTheme theme(id, QString(":/icons/%1").arg(id));
        if (!theme.load()) {
            themeWarningLog() << "Couldn't load built-in icon theme" << id;
            continue;
        }

        addIconTheme(std::move(theme));
        themeDebugLog() << "Loaded Built-In Icon Theme" << id;
    }

    if (!m_iconThemeFolder.mkpath("."))
        themeWarningLog() << "Couldn't create icon theme folder";
    themeDebugLog() << "Icon Theme Folder Path:" << m_iconThemeFolder.absolutePath();

    QDirIterator directoryIterator(m_iconThemeFolder.path(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (directoryIterator.hasNext()) {
        QDir dir(directoryIterator.next());
        IconTheme theme(dir.dirName(), dir.path());
        if (!theme.load())
            continue;

        addIconTheme(std::move(theme));
        themeDebugLog() << "Loaded Custom Icon Theme from" << dir.path();
    }

    themeDebugLog() << "<> Icon themes initialized.";
}

void ThemeManager::initializeWidgets()
{
    themeDebugLog() << "<> Initializing Widget Themes";
    themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<SpikyMCTheme>());

    auto baseTheme = getTheme("spikymc");
    if (!baseTheme)
        themeWarningLog() << "Couldn't load base theme for custom themes";

    // Load custom themes from the theme folder
    if (!m_applicationThemeFolder.mkpath("."))
        themeWarningLog() << "Couldn't create application theme folder";
    themeDebugLog() << "Application Theme Folder Path:" << m_applicationThemeFolder.absolutePath();

    QDirIterator customThemeIterator(m_applicationThemeFolder.path(), QDir::Dirs | QDir::Files, QDirIterator::Subdirectories);
    while (customThemeIterator.hasNext()) {
        QFileInfo file(customThemeIterator.next());
        if (file.fileName() == QStringLiteral("theme.json")) {
            if (!baseTheme)
                continue;

            themeDebugLog() << "Loading Custom Theme from manifest:" << file.filePath();
            addTheme(std::make_unique<CustomTheme>(baseTheme, file, true));
        } else if (file.suffix() == QStringLiteral("qss")) {
            if (!baseTheme)
                continue;

            themeDebugLog() << "Loading Custom Theme from qss:" << file.filePath();
            addTheme(std::make_unique<CustomTheme>(baseTheme, file, false));
        }
    }

    themeDebugLog() << "<> Widget themes initialized.";
}

#ifndef Q_OS_MACOS
void ThemeManager::setTitlebarColorOnMac(WId windowId, QColor color) {}
void ThemeManager::setTitlebarColorOfAllWindowsOnMac(QColor color) {}
void ThemeManager::stopSettingNewWindowColorsOnMac() {}
#endif

QList<IconTheme*> ThemeManager::getValidIconThemes()
{
    QList<IconTheme*> ret;
    ret.reserve(m_icons.size());
    for (auto&& [id, theme] : m_icons) {
        ret.append(&theme);
    }
    return ret;
}

QList<ITheme*> ThemeManager::getValidApplicationThemes()
{
    QList<ITheme*> ret;
    ret.reserve(m_themes.size());
    for (auto&& [id, theme] : m_themes) {
        ret.append(theme.get());
    }
    return ret;
}



bool ThemeManager::isValidIconTheme(const QString& id)
{
    return !id.isEmpty() && m_icons.find(id) != m_icons.end();
}

bool ThemeManager::isValidApplicationTheme(const QString& id)
{
    return !id.isEmpty() && m_themes.find(id) != m_themes.end();
}

QDir ThemeManager::getIconThemesFolder()
{
    return m_iconThemeFolder;
}

QDir ThemeManager::getApplicationThemesFolder()
{
    return m_applicationThemeFolder;
}



void ThemeManager::setIconTheme(const QString& name)
{
    if (m_icons.find(name) == m_icons.end()) {
        themeWarningLog() << "Tried to set invalid icon theme:" << name;
        return;
    }

    QIcon::setThemeName(name);
}

void ThemeManager::setApplicationTheme(const QString& name, bool initial)
{
    auto systemPalette = qApp->palette();
    auto themeIter = m_themes.find(name);
    if (themeIter != m_themes.end()) {
        auto& theme = themeIter->second;
        themeDebugLog() << "applying theme" << theme->name();
        theme->apply(initial);
        setTitlebarColorOfAllWindowsOnMac(qApp->palette().window().color());

        m_logColors = theme->logColorScheme();
    } else {
        themeWarningLog() << "Tried to set invalid theme:" << name;
    }
}

void ThemeManager::applyCurrentlySelectedTheme(bool initial)
{
    auto settings = APPLICATION->settings();
    setIconTheme(settings->get("IconTheme").toString());
    themeDebugLog() << "<> Icon theme set.";
    auto applicationTheme = settings->get("ApplicationTheme").toString();
    if (applicationTheme == "") {
        applicationTheme = "material";
    }
    setApplicationTheme(applicationTheme, initial);
    themeDebugLog() << "<> Application theme set.";
}



void ThemeManager::refresh()
{
    m_themes.clear();
    m_icons.clear();
    initializeThemes();
}

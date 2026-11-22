// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  Search index for the settings dialog: one query matches the setting ID,
 *  the localized text currently shown and the English source strings.
 */

#include "SettingsSearch.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QGridLayout>
#include <QHash>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSet>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextDocumentFragment>
#include <QWidget>
#include <QXmlStreamReader>

#include <algorithm>
#include <limits>

#include "ui/pages/BasePage.h"

// The .ui files live in a static library; referencing the initializer keeps them linked in.
void initSettingsUiResources()
{
    Q_INIT_RESOURCE(settingsui);
}

namespace {

// Setting ID -> widget in the page. Only needed so that searching by the
// technical key jumps to the right control; text search works without it.
struct IdEntry {
    const char* page;
    const char* widget;
    const char* id;
};

const IdEntry ID_TABLE[] = {
    // launcher-settings
    { "launcher-settings", "preferMenuBarCheckBox", "MenuBarInsteadOfToolBar" },
    { "launcher-settings", "numberOfConcurrentTasksSpinBox", "NumberOfConcurrentTasks" },
    { "launcher-settings", "numberOfConcurrentDownloadsSpinBox", "NumberOfConcurrentDownloads" },
    { "launcher-settings", "numberOfManualRetriesSpinBox", "NumberOfManualRetries" },
    { "launcher-settings", "timeoutSecondsSpinBox", "RequestTimeout" },
    { "launcher-settings", "lineLimitSpinBox", "ConsoleMaxLines" },
    { "launcher-settings", "checkStopLogging", "ConsoleOverflowStop" },
    { "launcher-settings", "instDirTextBox", "InstanceDir" },
    { "launcher-settings", "modsDirTextBox", "CentralModsDir" },
    { "launcher-settings", "iconsDirTextBox", "IconsDir" },
    { "launcher-settings", "downloadsDirTextBox", "DownloadsDir" },
    { "launcher-settings", "skinsDirTextBox", "SkinsDir" },
    { "launcher-settings", "javaDirTextBox", "JavaDir" },
    { "launcher-settings", "downloadsDirWatchRecursiveCheckBox", "DownloadsDirWatchRecursive" },
    { "launcher-settings", "downloadsDirMoveCheckBox", "MoveModsFromDownloadsDir" },
    { "launcher-settings", "editInstanceOnDoubleClick", "EditInstanceOnDoubleClick" },
    { "launcher-settings", "metadataEnableBtn", "ModMetadataDisabled" },
    { "launcher-settings", "dependenciesEnableBtn", "ModDependenciesDisabled" },
    { "launcher-settings", "showModIncompatCheckBox", "ShowModIncompat" },
    { "launcher-settings", "modpackUpdatePromptBtn", "SkipModpackUpdatePrompt" },
    { "launcher-settings", "downloadGameFilesBtn", "DownloadGameFilesDuringInstanceCreation" },
    // apis
    { "apis", "pasteTypeComboBox", "PastebinType" },
    { "apis", "baseURLEntry", "PastebinCustomAPIBase" },
    { "apis", "msaClientID", "MSAClientIDOverride" },
    { "apis", "elyClientID", "ElyClientIDOverride" },
    { "apis", "FallbackMRBlockedMods", "FallbackMRBlockedMods" },
    { "apis", "metaURL", "MetaURLOverride" },
    { "apis", "metaRefreshOnLaunchCB", "MetaRefreshOnLaunch" },
    { "apis", "resourceURL", "ResourceURLOverride" },
    { "apis", "flameKey", "FlameKeyOverride" },
    { "apis", "modrinthToken", "ModrinthToken" },
    { "apis", "userAgentLineEdit", "UserAgentOverride" },
    { "apis", "technicClientID", "TechnicClientID" },
    // external-tools
    { "external-tools", "jprofilerPathEdit", "JProfilerPath" },
    { "external-tools", "jvisualvmPathEdit", "JVisualVMPath" },
    // proxy-settings
    { "proxy-settings", "proxyTypeBox", "ProxyType" },
    { "proxy-settings", "proxyAddrEdit", "ProxyAddr" },
    { "proxy-settings", "proxyPortEdit", "ProxyPort" },
    { "proxy-settings", "proxyUserEdit", "ProxyUser" },
    { "proxy-settings", "proxyPassEdit", "ProxyPass" },
    // java-settings
    { "java-settings", "javaPathTextBox", "JavaPath" },
    { "java-settings", "skipCompatibilityCheckBox", "IgnoreJavaCompatibility" },
    { "java-settings", "skipWizardCheckBox", "IgnoreJavaWizard" },
    { "java-settings", "autodetectJavaCheckBox", "AutomaticJavaSwitch" },
    { "java-settings", "autodownloadJavaCheckBox", "AutomaticJavaDownload" },
    { "java-settings", "minMemSpinBox", "MinMemAlloc" },
    { "java-settings", "maxMemSpinBox", "MaxMemAlloc" },
    { "java-settings", "permGenSpinBox", "PermGen" },
    { "java-settings", "lowMemWarningCheckBox", "LowMemWarning" },
    { "java-settings", "jvmArgsTextBox", "JvmArgs" },
    // minecraft-settings
    { "minecraft-settings", "showGameTime", "ShowGameTime" },
    { "minecraft-settings", "recordGameTime", "RecordGameTime" },
    { "minecraft-settings", "countGameTime", "CountGameTime" },
    { "minecraft-settings", "showGlobalGameTime", "ShowGlobalGameTime" },
    { "minecraft-settings", "showGameTimeWithoutDays", "ShowGameTimeWithoutDays" },
    { "minecraft-settings", "maximizedCheckBox", "LaunchMaximized" },
    { "minecraft-settings", "windowWidthSpinBox", "MinecraftWinWidth" },
    { "minecraft-settings", "windowHeightSpinBox", "MinecraftWinHeight" },
    { "minecraft-settings", "closeAfterLaunchCheck", "CloseAfterLaunch" },
    { "minecraft-settings", "quitAfterGameStopCheck", "QuitAfterGameStop" },
    { "minecraft-settings", "showConsoleCheck", "ShowConsole" },
    { "minecraft-settings", "autoCloseConsoleCheck", "AutoCloseConsole" },
    { "minecraft-settings", "showConsoleErrorCheck", "ShowConsoleOnError" },
    { "minecraft-settings", "serverJoinAddress", "JoinServerOnLaunchAddress" },
    { "minecraft-settings", "lineEditGLFWPath", "CustomGLFWPath" },
    { "minecraft-settings", "lineEditOpenALPath", "CustomOpenALPath" },
    { "minecraft-settings", "lineEditSDLPath", "CustomSDLPath" },
    { "minecraft-settings", "onlineFixes", "OnlineFixes" },
    { "minecraft-settings", "useNativeGLFWCheck", "UseNativeGLFW" },
    { "minecraft-settings", "useNativeOpenALCheck", "UseNativeOpenAL" },
    { "minecraft-settings", "useNativeSDLCheck", "UseNativeSDL" },
    // appearance-settings
    { "appearance-settings", "iconsComboBox", "IconTheme" },
    { "appearance-settings", "widgetStyleComboBox", "ApplicationTheme" },
    { "appearance-settings", "consoleFont", "ConsoleFont" },
    { "appearance-settings", "fontSizeBox", "ConsoleFontSize" },
    // language-settings
    { "language-settings", "languageView", "Language" },
};

struct UiFile {
    const char* page;
    const char* path;
};

// English source strings live in the .ui files. Map them per page, otherwise common
// object names like "label" pick up strings from unrelated pages.
const UiFile UI_FILES[] = {
    { "launcher-settings", ":/settingsui/LauncherPage.ui" },
    { "apis", ":/settingsui/APIPage.ui" },
    { "external-tools", ":/settingsui/ExternalToolsPage.ui" },
    { "java-settings", ":/settingsui/JavaPage.ui" },
    { "java-settings", ":/settingsui/JavaSettingsWidget.ui" },
    { "proxy-settings", ":/settingsui/ProxyPage.ui" },
    { "accounts", ":/settingsui/AccountListPage.ui" },
    { "minecraft-settings", ":/settingsui/MinecraftSettingsWidget.ui" },
    { "minecraft-settings", ":/settingsui/CustomCommands.ui" },
    { "minecraft-settings", ":/settingsui/EnvironmentVariables.ui" },
    { "appearance-settings", ":/settingsui/AppearanceWidget.ui" },
};

QString clean(QString text)
{
    if (Qt::mightBeRichText(text)) {
        text = QTextDocumentFragment::fromHtml(text).toPlainText();
    }
    text.remove(QLatin1Char('&'));
    return text.simplified();
}

/// English label/title strings from the .ui files of one page, keyed by widget object name.
const QHash<QString, QStringList>& englishStrings(const QString& pageId)
{
    static QHash<QString, QHash<QString, QStringList>> cache;
    auto cached = cache.constFind(pageId);
    if (cached != cache.constEnd()) {
        return *cached;
    }

    initSettingsUiResources();
    QHash<QString, QStringList> result;
    for (const auto& entry : UI_FILES) {
        if (pageId != QLatin1String(entry.page)) {
            continue;
        }
        QFile file(QString::fromLatin1(entry.path));
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Settings index: cannot open" << entry.path;
            continue;
        }
        qDebug() << "Settings index: reading" << entry.path << file.size();
        QXmlStreamReader xml(&file);
        QString widgetName;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == QLatin1String("widget")) {
                    widgetName = xml.attributes().value(QLatin1String("name")).toString();
                } else if (xml.name() == QLatin1String("property") && !widgetName.isEmpty()) {
                    const auto property = xml.attributes().value(QLatin1String("name")).toString();
                    if (property == QLatin1String("text") || property == QLatin1String("title")) {
                        const QString value = clean(xml.readElementText(QXmlStreamReader::IncludeChildElements));
                        if (!value.isEmpty()) {
                            result[widgetName].append(value);
                        }
                    }
                }
            }
        }
    }
    return *cache.insert(pageId, result);
}

/// Only real settings: checkboxes and radio buttons with their own text. Push buttons,
/// group headers and descriptions are actions/decoration and only add noise to results.
bool ownLabel(QWidget* widget, QString& text)
{
    if (qobject_cast<QCheckBox*>(widget) || qobject_cast<QRadioButton*>(widget)) {
        text = clean(static_cast<QAbstractButton*>(widget)->text());
        return !text.isEmpty();
    }
    return false;
}

/// Label next to a control, taken from the layout structure (works even when the page
/// has never been shown and therefore has no widget geometry yet).
QLabel* labelInLayout(QWidget* widget, QLayout* layout)
{
    if (auto* grid = qobject_cast<QGridLayout*>(layout)) {
        const int index = grid->indexOf(widget);
        if (index < 0) {
            return nullptr;
        }
        int row = 0, column = 0, rowSpan = 1, columnSpan = 1;
        grid->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);

        QLabel* sameRow = nullptr;
        int bestColumn = -1;
        QLabel* above = nullptr;
        for (int i = 0; i < grid->count(); ++i) {
            auto* item = grid->itemAt(i);
            if (!item) {
                continue;
            }
            auto* label = qobject_cast<QLabel*>(item->widget());
            if (!label || label->text().isEmpty()) {
                continue;
            }
            // Skip description/warning paragraphs; they describe the field, they are not its name.
            const QString name = label->objectName();
            if (name.contains(QLatin1String("escription")) || name.contains(QLatin1String("arning"))) {
                continue;
            }
            int r = 0, c = 0, rs = 1, cs = 1;
            grid->getItemPosition(i, &r, &c, &rs, &cs);
            if (r == row && c < column && c > bestColumn) {
                bestColumn = c;
                sameRow = label;
            } else if (r == row - 1 && c == column) {
                above = label;
            }
        }
        return sameRow ? sameRow : above;
    }

    if (auto* box = qobject_cast<QBoxLayout*>(layout)) {
        for (int i = box->indexOf(widget) - 1; i >= 0; --i) {
            auto* item = box->itemAt(i);
            if (!item) {
                continue;
            }
            if (auto* label = qobject_cast<QLabel*>(item->widget())) {
                const QString labelName = label->objectName();
                if (!label->text().isEmpty() && !labelName.contains(QLatin1String("escription")) &&
                    !labelName.contains(QLatin1String("arning"))) {
                    return label;
                }
            }
        }
    }
    return nullptr;
}

/// Label that belongs to a control: the .ui buddy first (exact), then the enclosing layout.
QLabel* labelWidgetFor(QWidget* widget, QWidget* scope)
{
    if (!scope) {
        return nullptr;
    }
    for (auto* label : scope->findChildren<QLabel*>()) {
        if (label->buddy() == widget) {
            return label;
        }
    }
    for (QWidget* parent = widget->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto* label = labelInLayout(widget, parent->layout())) {
            return label;
        }
        if (parent == scope) {
            break;
        }
    }
    return nullptr;
}

bool isInputWidget(QWidget* widget)
{
    return qobject_cast<QLineEdit*>(widget) || qobject_cast<QAbstractSpinBox*>(widget) ||
           qobject_cast<QComboBox*>(widget) || qobject_cast<QPlainTextEdit*>(widget) ||
           qobject_cast<QTextEdit*>(widget) || qobject_cast<QListWidget*>(widget) ||
           qobject_cast<QSlider*>(widget) || qobject_cast<QKeySequenceEdit*>(widget);
}

QString normalize(const QString& text)
{
    QString result = text.toLower();
    result.remove(QRegularExpression(QStringLiteral("[\\s_\\-./\\\\()\\[\\]:,]+")));
    return result;
}

}  // namespace

void SettingsSearch::build(const QList<BasePage*>& pages)
{
    m_entries.clear();

    for (auto* page : pages) {
        auto* pageWidget = dynamic_cast<QWidget*>(page);
        if (!pageWidget) {
            continue;
        }
        // Local copy of the id: GCC's -Wdangling-reference trips on temporaries here.
        const QString pageId = page->id();
        const auto english = englishStrings(pageId);

        // The page itself is a result too, so typing the section name finds it.
        Entry pageEntry;
        pageEntry.page = page;
        pageEntry.title = page->displayName();
        pageEntry.id = page->id();
        m_entries.append(pageEntry);

        for (auto* widget : pageWidget->findChildren<QWidget*>()) {
            QString own;
            if (ownLabel(widget, own)) {
                Entry entry;
                entry.page = page;
                entry.target = widget;
                entry.title = own;
                entry.english = english.value(widget->objectName()).join(QLatin1String(" | "));
                m_entries.append(entry);
                continue;
            }
            if (!isInputWidget(widget)) {
                continue;
            }
            auto* label = labelWidgetFor(widget, pageWidget);
            if (!label) {
                continue;
            }
            Entry entry;
            entry.page = page;
            entry.target = widget;
            entry.title = clean(label->text());
            entry.english = english.value(label->objectName()).join(QLatin1String(" | "));
            if (entry.english.isEmpty()) {
                entry.english = english.value(widget->objectName()).join(QLatin1String(" | "));
            }
            if (entry.title.isEmpty() && entry.english.isEmpty()) {
                continue;
            }
            m_entries.append(entry);
        }
    }

    // Attach technical IDs to the controls they belong to.
    for (const auto& row : ID_TABLE) {
        BasePage* page = nullptr;
        for (auto* candidate : pages) {
            if (candidate->id() == QLatin1String(row.page)) {
                page = candidate;
                break;
            }
        }
        if (!page) {
            continue;
        }
        auto* pageWidget = dynamic_cast<QWidget*>(page);
        if (!pageWidget) {
            continue;
        }
        auto* widget = pageWidget->findChild<QWidget*>(QString::fromLatin1(row.widget));
        if (!widget) {
            continue;
        }

        Entry* existing = nullptr;
        for (auto& entry : m_entries) {
            if (entry.page == page && entry.target == widget) {
                existing = &entry;
                break;
            }
        }
        if (existing) {
            existing->id = QString::fromLatin1(row.id);
            continue;
        }

        Entry entry;
        entry.page = page;
        entry.target = widget;
        entry.id = QString::fromLatin1(row.id);
        const QString pageId = page->id();
        const auto english = englishStrings(pageId);
        entry.english = english.value(widget->objectName()).join(QLatin1String(" | "));
        if (auto* label = labelWidgetFor(widget, pageWidget)) {
            entry.title = clean(label->text());
            if (entry.english.isEmpty()) {
                entry.english = english.value(label->objectName()).join(QLatin1String(" | "));
            }
        }
        if (entry.title.isEmpty()) {
            entry.title = english.value(widget->objectName()).value(0);
        }
        if (entry.title.isEmpty()) {
            entry.title = page->displayName();
        }
        m_entries.append(entry);
    }
}

QList<SettingsSearch::Entry> SettingsSearch::query(const QString& text, int limit) const
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    QStringList tokens;
    for (const auto& raw : trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts)) {
        const QString token = normalize(raw);
        if (!token.isEmpty()) {
            tokens.append(token);
        }
    }
    if (tokens.isEmpty()) {
        return {};
    }

    struct Scored {
        const Entry* entry;
        int score;
    };
    QList<Scored> scored;

    for (const auto& entry : m_entries) {
        // Page name only matches the page entry itself, not every setting inside it.
        const QString hay = normalize(entry.target ? entry.title + QLatin1Char(' ') + entry.english + QLatin1Char(' ') + entry.id
                                                   : entry.page->displayName() + QLatin1Char(' ') + entry.id);

        bool all = true;
        for (const auto& token : tokens) {
            if (!hay.contains(token)) {
                all = false;
                break;
            }
        }
        if (!all) {
            continue;
        }

        int score = 50;
        const QString idNorm = normalize(entry.id);
        const QString englishNorm = normalize(entry.english);
        const QString titleNorm = normalize(entry.title);
        bool ids = !idNorm.isEmpty();
        bool en = !englishNorm.isEmpty();
        bool ti = !titleNorm.isEmpty();
        for (const auto& token : tokens) {
            ids = ids && idNorm.contains(token);
            en = en && englishNorm.contains(token);
            ti = ti && titleNorm.contains(token);
        }
        if (ids) {
            score = 0;
        } else if (en) {
            score = 10;
        } else if (ti) {
            score = 20;
        }
        // Prefer concrete settings over whole pages at the same score.
        if (!entry.target) {
            score += 5;
        }
        scored.append({ &entry, score });
    }

    std::sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
        if (a.score != b.score) {
            return a.score < b.score;
        }
        return a.entry->title.localeAwareCompare(b.entry->title) < 0;
    });

    QList<Entry> result;
    QSet<const QWidget*> seen;
    QSet<QString> seenKeys;
    for (const auto& item : scored) {
        if (item.entry->target && seen.contains(item.entry->target)) {
            continue;
        }
        // Collapse identical rows coming from several controls with the same caption.
        const QString key = (item.entry->page->id() + QLatin1Char('|') + item.entry->title);
        if (item.entry->target) {
            if (seenKeys.contains(key)) {
                continue;
            }
            seenKeys.insert(key);
            seen.insert(item.entry->target);
        }
        result.append(*item.entry);
        if (result.size() >= limit) {
            break;
        }
    }
    return result;
}

// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "PageContainer.h"
#include "BuildConfig.h"
#include "PageContainer_p.h"
#include "SettingsSearch.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <utility>

#include "settings/SettingsObject.h"

#include "ui/widgets/IconLabel.h"

#include "Application.h"
#include "DesktopServices.h"

class PageEntryFilterModel : public QSortFilterProxyModel {
   public:
    explicit PageEntryFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const QString pattern = filterRegularExpression().pattern();
        auto* const model = static_cast<PageModel*>(sourceModel());
        auto* const page = model->pages().at(sourceRow);
        if (!page->shouldDisplay()) {
            return false;
        }
        // Regular contents check, then check page-filter.
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

/// Two-line item: matched text on top, section name below (no technical key).
class SearchResultDelegate : public QStyledItemDelegate {
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        return QSize(0, 44);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        const QWidget* widget = option.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

        const bool selected = option.state & QStyle::State_Selected;
        const QRect rect = option.rect.adjusted(10, 6, -10, -6);

        painter->save();
        QFont titleFont = option.font;
        painter->setFont(titleFont);
        painter->setPen(option.palette.color(selected ? QPalette::HighlightedText : QPalette::Text));
        const QFontMetrics titleMetrics(titleFont);
        painter->drawText(QRect(rect.left(), rect.top(), rect.width(), titleMetrics.height()), Qt::AlignLeft | Qt::AlignVCenter,
                          titleMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, rect.width()));

        QFont subtitleFont = titleFont;
        if (subtitleFont.pointSizeF() > 0) {
            subtitleFont.setPointSizeF(subtitleFont.pointSizeF() * 0.88);
        }
        painter->setFont(subtitleFont);
        painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));
        const QFontMetrics subtitleMetrics(subtitleFont);
        painter->drawText(QRect(rect.left(), rect.top() + titleMetrics.height() + 1, rect.width(), subtitleMetrics.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          subtitleMetrics.elidedText(index.data(Qt::UserRole + 1).toString(), Qt::ElideRight, rect.width()));
        painter->restore();
    }
};

PageContainer::PageContainer(BasePageProvider* pageProvider, QString defaultId, QWidget* parent)
    : QWidget(parent)
    , m_proxyModel(new PageEntryFilterModel(this))
    , m_model(new PageModel(this))
{
    createUI();
    useSidebarStyle(true);

    int counter = 0;
    auto pages = pageProvider->getPages();
    for (auto* page : pages) {
        auto* widget = dynamic_cast<QWidget*>(page);
        widget->setParent(this);
        page->stackIndex = m_pageStack->addWidget(widget);
        page->listIndex = counter;
        page->setParentContainer(this);
        counter++;
        page->updateExtraInfo = [this](const QString& id, const QString& info) {
            if (m_currentPage && id == m_currentPage->id()) {
                m_header->setText(m_currentPage->displayName() + info);
            }
        };
    }
    m_model->setPages(pages);

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
    m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_pageList->setModel(m_proxyModel);
    connect(m_pageList->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &PageContainer::currentChanged);
    m_pageStack->setStackingMode(QStackedLayout::StackOne);
    m_pageList->setFocus();
    selectPage(std::move(defaultId));
}

bool PageContainer::selectPage(QString pageId)
{
    // now find what we want to have selected...
    auto* page = m_model->findPageEntryById(pageId);
    QModelIndex index;
    if (page) {
        index = m_proxyModel->mapFromSource(m_model->index(page->listIndex));
    }
    if (!index.isValid()) {
        index = m_proxyModel->index(0, 0);
    }
    if (index.isValid()) {
        m_pageList->setCurrentIndex(index);
        return true;
    }
    return false;
}

BasePage* PageContainer::getPage(QString pageId)
{
    return m_model->findPageEntryById(pageId);
}

BasePage* PageContainer::selectedPage() const
{
    return m_currentPage;
}

const QList<BasePage*>& PageContainer::getPages() const
{
    return m_model->pages();
}

void PageContainer::refreshContainer()
{
    m_proxyModel->invalidate();
    if (!m_currentPage->shouldDisplay()) {
        auto index = m_proxyModel->index(0, 0);
        if (index.isValid()) {
            m_pageList->setCurrentIndex(index);
        } else {
            // FIXME: unhandled corner case: what to do when there's no page to select?
        }
    }
}

void PageContainer::createUI()
{
    m_pageStack = new QStackedLayout;
    m_pageList = new PageView;
    m_header = new QLabel();

    QFont headerLabelFont = m_header->font();
    headerLabelFont.setBold(true);
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0) {
        headerLabelFont.setPointSize(pointSize + 2);
    }
    m_header->setFont(headerLabelFont);

    auto* headerHLayout = new QHBoxLayout;
    const int leftMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_header);
    headerHLayout->setContentsMargins(0, 6, 0, 0);

    m_pageStack->setContentsMargins(0, 0, 0, 0);
    m_pageStack->addWidget(new QWidget(this));

    m_layout = new QGridLayout;
    m_layout->addLayout(headerHLayout, 0, 1, 1, 1);
    m_layout->addWidget(m_pageList, 1, 0, 3, 1);
    m_layout->addLayout(m_pageStack, 1, 1, 1, 1);
    m_layout->setColumnStretch(1, 4);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void PageContainer::retranslate()
{
    if (m_currentPage) {
        m_header->setText(m_currentPage->displayName());
    }

    for (auto* page : m_model->pages()) {
        page->retranslate();
    }

    if (m_searchEdit) {
        m_searchEdit->setPlaceholderText(tr("Search settings…"));
    }
    rebuildSearchIndex();
}

void PageContainer::addButtons(QWidget* buttons)
{
    m_layout->addWidget(buttons, 2, 1, 1, 2);
}

void PageContainer::addButtons(QLayout* buttons)
{
    m_layout->addLayout(buttons, 2, 1, 1, 2);
}

void PageContainer::useSidebarStyle(bool sidebar)
{
    m_pageList->setProperty("_kde_side_panel_view", sidebar);
}

void PageContainer::setSearchEnabled(bool enabled)
{
    if (m_searchEnabled == enabled) {
        return;
    }
    m_searchEnabled = enabled;

    if (!enabled) {
        if (m_searchEdit) {
            m_searchEdit->hide();
        }
        if (m_searchResults) {
            m_searchResults->hide();
        }
        return;
    }

    if (!m_search) {
        m_search = std::make_unique<SettingsSearch>();
    }

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search settings…"));
    m_searchEdit->setClearButtonEnabled(true);
    m_layout->addWidget(m_searchEdit, 0, 0, 1, 1);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PageContainer::applySearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this] {
        if (m_searchResults && m_searchResults->currentRow() >= 0) {
            activateSearchResult(m_searchResults->currentRow());
        }
    });

    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this] {
        if (m_searchEdit) {
            m_searchEdit->setFocus();
            m_searchEdit->selectAll();
        }
    });

    m_searchResults = new QListWidget(this);
    m_searchResults->setFrameShape(QFrame::NoFrame);
    m_searchResults->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_searchResults->setSelectionMode(QAbstractItemView::SingleSelection);
    m_searchResults->setUniformItemSizes(false);
    m_searchResults->setItemDelegate(new SearchResultDelegate(m_searchResults));
    m_searchResults->hide();
    m_layout->addWidget(m_searchResults, 1, 0, 3, 1);
    connect(m_searchResults, &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) { activateSearchResult(m_searchResults->row(item)); });
    connect(m_searchResults, &QListWidget::itemActivated, this,
            [this](QListWidgetItem* item) { activateSearchResult(m_searchResults->row(item)); });

    m_searchEdit->installEventFilter(this);
    rebuildSearchIndex();
    m_searchEdit->setFocus(Qt::OtherFocusReason);
}

void PageContainer::rebuildSearchIndex()
{
    if (!m_searchEnabled || !m_search) {
        return;
    }
    m_search->build(getPages());
    if (m_searchEdit && !m_searchEdit->text().trimmed().isEmpty()) {
        applySearch(m_searchEdit->text());
    }
}

void PageContainer::applySearch(const QString& text)
{
    if (!m_searchResults) {
        return;
    }
    m_currentResults = m_search->query(text);
    m_searchResults->clear();

    if (text.trimmed().isEmpty()) {
        m_searchResults->hide();
        m_pageList->show();
        return;
    }

    m_pageList->hide();
    m_searchResults->show();

    if (m_currentResults.isEmpty()) {
        auto* item = new QListWidgetItem(m_searchResults);
        item->setFlags(Qt::NoItemFlags);
        item->setText(tr("Nothing found"));
        item->setForeground(palette().color(QPalette::Disabled, QPalette::Text));
        return;
    }

    for (const auto& entry : m_currentResults) {
        auto* item = new QListWidgetItem(m_searchResults);
        item->setText(entry.title);
        item->setData(Qt::UserRole + 1, entry.page ? entry.page->displayName() : QString());
        item->setSizeHint(QSize(0, 44));
    }
    m_searchResults->setCurrentRow(0);
}

bool PageContainer::eventFilter(QObject* watched, QEvent* event)
{
    if (m_searchEdit && watched == m_searchEdit && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Down && m_searchResults && m_searchResults->count() > 0) {
            m_searchResults->setFocus(Qt::OtherFocusReason);
            if (m_searchResults->currentRow() < 0) {
                m_searchResults->setCurrentRow(0);
            }
            return true;
        }
        if (key->key() == Qt::Key_Escape && !m_searchEdit->text().isEmpty()) {
            m_searchEdit->clear();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PageContainer::activateSearchResult(int row)
{
    if (row < 0 || row >= m_currentResults.size()) {
        return;
    }
    const auto entry = m_currentResults.at(row);
    if (!entry.page) {
        return;
    }
    selectPage(entry.page->id());
    if (!entry.target) {
        return;
    }
    QPointer<QWidget> target = entry.target;
    QTimer::singleShot(0, this, [this, target] {
        if (target) {
            revealResult(target);
        }
    });
}

void PageContainer::revealResult(QWidget* target)
{
    // Switch the inner tab that owns the setting, e.g. Minecraft -> "Custom commands".
    bool switchedTab = false;
    for (QWidget* parent = target->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto* tabs = qobject_cast<QTabWidget*>(parent)) {
            for (int i = 0; i < tabs->count(); ++i) {
                if (tabs->widget(i)->isAncestorOf(target)) {
                    if (tabs->currentIndex() != i) {
                        tabs->setCurrentIndex(i);
                        switchedTab = true;
                    }
                    break;
                }
            }
            break;
        }
    }

    QPointer<QWidget> guard = target;
    if (switchedTab) {
        // Give the page a layout pass before scrolling to the now visible widget.
        QTimer::singleShot(0, this, [this, guard] {
            if (guard) {
                revealResultNow(guard);
            }
        });
        return;
    }
    revealResultNow(target);
}

void PageContainer::revealResultNow(QWidget* target)
{
    QWidget* scrollArea = target->parentWidget();
    while (scrollArea) {
        if (auto* area = qobject_cast<QScrollArea*>(scrollArea)) {
            area->ensureWidgetVisible(target, 24, 24);
            break;
        }
        scrollArea = scrollArea->parentWidget();
    }

    if (target->focusPolicy() != Qt::NoFocus) {
        target->setFocus(Qt::OtherFocusReason);
    }

    // Border on the widget itself, so the mark follows it through scroll and layout changes.
    const QString oldStyle = target->styleSheet();
    const auto oldPalette = target->palette();
    target->setStyleSheet(oldStyle + QStringLiteral("\nborder: 2px solid rgb(108, 192, 122); border-radius: 3px;"));
    QPalette marked = oldPalette;
    marked.setColor(QPalette::Base, QColor(108, 192, 122, 55));
    marked.setColor(QPalette::Window, QColor(108, 192, 122, 55));
    target->setPalette(marked);

    QPointer<QWidget> guard = target;
    QTimer::singleShot(1300, target, [guard, oldStyle, oldPalette] {
        if (guard) {
            guard->setStyleSheet(oldStyle);
            guard->setPalette(oldPalette);
        }
    });
}

void PageContainer::showPage(int row)
{
    if (m_currentPage) {
        m_currentPage->closed();
    }
    if (row != -1) {
        m_currentPage = m_model->pages().at(row);
    } else {
        m_currentPage = nullptr;
    }
    if (m_currentPage) {
        m_pageStack->setCurrentIndex(m_currentPage->stackIndex);
        m_header->setText(m_currentPage->displayName());
        m_currentPage->opened();
    } else {
        m_pageStack->setCurrentIndex(0);
        m_header->setText(QString());
    }
}

void PageContainer::currentChanged(const QModelIndex& current)
{
    int selectedIndex = current.isValid() ? m_proxyModel->mapToSource(current).row() : -1;

    auto* selected = m_model->pages().at(selectedIndex);
    auto* previous = m_currentPage;

    emit selectedPageChanged(previous, selected);

    showPage(selectedIndex);
}

bool PageContainer::prepareToClose()
{
    if (!saveAll()) {
        return false;
    }
    if (m_currentPage) {
        m_currentPage->closed();
    }
    return true;
}

bool PageContainer::saveAll()
{
    for (auto* page : m_model->pages()) {
        if (!page->apply()) {
            return false;
        }
    }
    return true;
}

void PageContainer::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}

// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *  Copyright (C) 2026 SpikyTeam
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
#include "SpikyMCTheme.h"

#include <QObject>

QString SpikyMCTheme::id()
{
    return "spikymc";
}

QString SpikyMCTheme::name()
{
    return QObject::tr("SpikyMC Theme");
}

QPalette SpikyMCTheme::colorScheme()
{
    QPalette materialPalette;
    // Максимально глубокий фон окна (#0D1210) для выделения поверхностей
    materialPalette.setColor(QPalette::Window, QColor(13, 18, 16));
    materialPalette.setColor(QPalette::WindowText, QColor(240, 244, 241));
    
    // Поверхности и инпуты (#121915)
    materialPalette.setColor(QPalette::Base, QColor(18, 25, 21));
    // Альтернативный фон для зебры в списках (#18221C)
    materialPalette.setColor(QPalette::AlternateBase, QColor(24, 34, 28));
    
    // Тултипы
    materialPalette.setColor(QPalette::ToolTipBase, QColor(18, 25, 21));
    materialPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    materialPalette.setColor(QPalette::Text, QColor(240, 244, 241));
    
    // Обычные кнопки (#1F2B24)
    materialPalette.setColor(QPalette::Button, QColor(31, 43, 36));
    materialPalette.setColor(QPalette::ButtonText, QColor(240, 244, 241));
    materialPalette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    
    // Фирменный акцентный цвет (#52A535)
    materialPalette.setColor(QPalette::Link, QColor(82, 165, 53));
    materialPalette.setColor(QPalette::Highlight, QColor(82, 165, 53));
    // На фоне #52A535 текст должен быть белым для читаемости
    materialPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255)); 
    
    // Приглушенный текст для плейсхолдеров (#738779)
    materialPalette.setColor(QPalette::PlaceholderText, QColor(115, 135, 121));
    
    return fadeInactive(materialPalette, fadeAmount(), fadeColor());
}

double SpikyMCTheme::fadeAmount()
{
    return 0.5;
}

QColor SpikyMCTheme::fadeColor()
{
    return QColor(13, 18, 16); // Затенение в цвет главного фона
}

bool SpikyMCTheme::hasStyleSheet()
{
    return true;
}

QString SpikyMCTheme::appStyleSheet()
{
    return R"QSS(
/* Базовые поверхности и тултипы */
QToolTip {
    color: #FFFFFF;
    background-color: #121915;
    border: 1px solid #52A535;
    border-radius: 6px;
    padding: 6px 10px;
    font-weight: bold;
}

/* Обычные кнопки - глубокие, с плавным переходом рамки */
QPushButton {
    color: #F0F4F1;
    background-color: #1F2B24;
    border: 1px solid #26362D;
    border-radius: 6px;
    padding: 6px 16px;
    outline: none;
}
QPushButton:hover {
    background-color: #28382E;
    border-color: #384D40;
    color: #FFFFFF;
}
QPushButton:pressed {
    background-color: #121915;
    border-color: #52A535;
}
QPushButton:disabled {
    color: #738779;
    background-color: #0D1210;
    border-color: #18221C;
}
QPushButton:focus {
    border: 1px solid #52A535;
}

/* Главная (акцентная) кнопка - фирменный цвет #52A535 */
QPushButton:default {
    color: #FFFFFF;
    background-color: #52A535;
    border: 1px solid #48912E;
    font-weight: bold;
}
QPushButton:default:hover {
    background-color: #63C742; /* Светлее для hover */
    border-color: #52A535;
}
QPushButton:default:pressed {
    background-color: #448A2C; /* Темнее для pressed */
    border-color: #3A7526;
}
QPushButton:default:disabled {
    color: #738779;
    background-color: #18221C;
    border-color: #26362D;
}

/* Кнопки на тулбаре */
QToolButton {
    color: #F0F4F1;
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 4px;
}
QToolButton:hover {
    background-color: #1F2B24;
    border-color: #26362D;
}
QToolButton:pressed {
    background-color: #121915;
    border-color: #52A535;
}
QToolButton:checked {
    color: #FFFFFF;
    background-color: #52A535;
}
QToolButton:disabled {
    color: #738779;
}

/* Поля ввода (Инпуты) */
QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox,
QDateTimeEdit, QDateEdit, QTimeEdit, QFontComboBox {
    color: #F0F4F1;
    background-color: #121915;
    border: 2px solid #26362D;
    border-radius: 6px;
    padding: 5px 10px;
    selection-background-color: #52A535;
    selection-color: #FFFFFF;
}
QLineEdit:hover, QTextEdit:hover, QPlainTextEdit:hover,
QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover,
QDateTimeEdit:hover, QDateEdit:hover, QTimeEdit:hover, QFontComboBox:hover {
    border-color: #384D40;
}
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus,
QDateTimeEdit:focus, QDateEdit:focus, QTimeEdit:focus, QFontComboBox:focus {
    border: 2px solid #52A535;
    background-color: #151D19;
}
QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled,
QDateTimeEdit:disabled, QDateEdit:disabled, QTimeEdit:disabled, QFontComboBox:disabled {
    color: #738779;
    background-color: #0D1210;
    border-color: #18221C;
}

/* Выпадающие списки */
QComboBox::drop-down {
    border: none;
    width: 24px;
    background-color: transparent;
}
QComboBox QAbstractItemView {
    color: #F0F4F1;
    background-color: #18221C;
    border: 1px solid #52A535;
    border-radius: 6px;
    selection-background-color: #52A535;
    selection-color: #FFFFFF;
    padding: 4px;
    outline: none;
}

/* Спинбоксы (Стрелочки) */
QSpinBox::up-button, QDoubleSpinBox::up-button,
QDateTimeEdit::up-button, QDateEdit::up-button, QTimeEdit::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button,
QDateTimeEdit::down-button, QDateEdit::down-button, QTimeEdit::down-button {
    width: 20px;
    background-color: transparent;
    border-left: 2px solid transparent;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QDateTimeEdit::up-button:hover, QDateEdit::up-button:hover, QTimeEdit::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover,
QDateTimeEdit::down-button:hover, QDateEdit::down-button:hover, QTimeEdit::down-button:hover {
    background-color: #1F2B24;
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow,
QDateTimeEdit::up-arrow, QDateEdit::up-arrow, QTimeEdit::up-arrow {
    width: 0; height: 0;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-bottom: 6px solid #F0F4F1;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow,
QDateTimeEdit::down-arrow, QDateEdit::down-arrow, QTimeEdit::down-arrow {
    width: 0; height: 0;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 6px solid #F0F4F1;
}

/* Чекбоксы и Радиокнопки */
QCheckBox, QRadioButton {
    color: #F0F4F1;
    spacing: 8px;
    padding: 4px;
}
QCheckBox:disabled, QRadioButton:disabled {
    color: #738779;
}
QCheckBox::indicator, QRadioButton::indicator {
    width: 18px;
    height: 18px;
    background-color: #121915;
    border: 2px solid #384D40;
}
QCheckBox::indicator {
    border-radius: 4px;
}
QRadioButton::indicator {
    border-radius: 11px;
}
QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border-color: #52A535;
}
QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background-color: #52A535;
    border-color: #52A535;
}
QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {
    background-color: #0D1210;
    border-color: #26362D;
}

/* Ползунки (Слайдеры) */
QSlider::groove:horizontal {
    height: 6px;
    background-color: #18221C;
    border-radius: 3px;
}
QSlider::sub-page:horizontal {
    background-color: #52A535;
    border-radius: 3px;
}
QSlider::handle:horizontal {
    width: 16px;
    height: 16px;
    margin: -5px 0;
    background-color: #FFFFFF;
    border: 2px solid #52A535;
    border-radius: 8px;
}
QSlider::handle:horizontal:hover {
    transform: scale(1.1);
    background-color: #63C742;
    border-color: #63C742;
}
QSlider::groove:vertical {
    width: 6px;
    background-color: #18221C;
    border-radius: 3px;
}
QSlider::sub-page:vertical {
    background-color: #52A535;
    border-radius: 3px;
}
QSlider::handle:vertical {
    width: 16px;
    height: 16px;
    margin: 0 -5px;
    background-color: #FFFFFF;
    border: 2px solid #52A535;
    border-radius: 8px;
}

/* Прогресс-бары */
QProgressBar {
    color: #FFFFFF;
    background-color: #121915;
    border: 1px solid #26362D;
    border-radius: 6px;
    text-align: center;
    font-weight: bold;
}
QProgressBar::chunk {
    background-color: #52A535;
    border-radius: 5px;
}

/* Скроллбары - плавающая капсула */
QScrollBar:vertical {
    background: transparent;
    width: 14px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background-color: #26362D;
    border-radius: 5px;
    min-height: 40px;
    margin: 0px 2px;
}
QScrollBar::handle:vertical:hover {
    background-color: #52A535;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}

QScrollBar:horizontal {
    background: transparent;
    height: 14px;
    margin: 2px;
}
QScrollBar::handle:horizontal {
    background-color: #26362D;
    border-radius: 5px;
    min-width: 40px;
    margin: 2px 0px;
}
QScrollBar::handle:horizontal:hover {
    background-color: #52A535;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: transparent;
}

/* Меню */
QMenu {
    color: #F0F4F1;
    background-color: #18221C;
    border: 1px solid #26362D;
    border-radius: 8px;
    padding: 6px;
}
QMenu::item {
    padding: 6px 24px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #52A535;
    color: #FFFFFF;
}
QMenu::item:disabled {
    color: #738779;
}
QMenu::separator {
    height: 1px;
    background-color: #26362D;
    margin: 6px 8px;
}

/* Панель меню (MenuBar) */
QMenuBar {
    color: #F0F4F1;
    background-color: transparent;
    spacing: 4px;
    padding: 2px 6px;
}
QMenuBar::item {
    padding: 6px 12px;
    border-radius: 6px;
    background-color: transparent;
}
QMenuBar::item:selected, QMenuBar::item:pressed {
    background-color: #1F2B24;
    color: #52A535;
}

QStatusBar {
    color: #738779;
    background-color: #0D1210;
}
QStatusBar::item {
    border: none;
}

/* Панели инструментов */
QToolBar {
    background-color: #0D1210;
    border: none;
    spacing: 4px;
    padding: 4px;
}
QToolBar::separator {
    background-color: #26362D;
    width: 1px;
    margin: 4px 6px;
}

QScrollArea, QAbstractScrollArea::corner {
    background-color: transparent;
    border: none;
}

QMainWindow, QDialog {
    background-color: #0D1210;
}

QDialogButtonBox QPushButton {
    min-width: 80px;
}

/* Группировка (GroupBox) */
QGroupBox {
    color: #F0F4F1;
    font-weight: bold;
    background-color: rgba(24, 34, 28, 0.3); /* Легкая заливка для объема */
    border: 1px solid #26362D;
    border-radius: 8px;
    margin-top: 14px;
    padding-top: 12px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 8px;
    color: #52A535;
    background-color: #0D1210; /* Фон под текстом заголовка */
    border-radius: 4px;
}

/* Вкладки (Tabs) */
QTabWidget::pane {
    border: 1px solid #26362D;
    border-radius: 8px;
    top: -1px;
    background: #0D1210;
}
QTabBar::tab {
    color: #738779;
    background-color: transparent;
    border: none;
    border-bottom: 2px solid transparent;
    padding: 10px 18px;
    font-weight: bold;
}
QTabBar::tab:hover {
    color: #F0F4F1;
    background-color: #121915;
}
QTabBar::tab:selected {
    color: #52A535;
    border-bottom: 2px solid #52A535;
}

/* Таблицы и Списки */
QHeaderView::section {
    color: #F0F4F1;
    background-color: #18221C;
    border: none;
    border-bottom: 2px solid #26362D;
    padding: 8px 12px;
    font-weight: bold;
}
QTableCornerButton::section {
    background-color: #18221C;
    border: none;
}

QSplitter::handle {
    background-color: transparent;
}
QSplitter::handle:horizontal {
    width: 6px;
}
QSplitter::handle:vertical {
    height: 6px;
}
QSplitter::handle:hover {
    background-color: #52A535;
}

QListView, QTableView, QTreeView {
    color: #F0F4F1;
    background-color: #121915;
    border: 1px solid #26362D;
    border-radius: 8px;
    outline: none;
}
QListView::item, QTableView::item, QTreeView::item {
    padding: 6px 8px;
    border-radius: 4px;
}
QListView::item:selected, QTableView::item:selected, QTreeView::item:selected {
    background-color: #52A535;
    color: #FFFFFF;
}
QListView::item:hover:!selected, QTableView::item:hover:!selected, QTreeView::item:hover:!selected {
    background-color: #1F2B24;
}
)QSS";
}

QString SpikyMCTheme::tooltip()
{
    return QString();
}
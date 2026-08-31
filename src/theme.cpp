#include "theme.h"

#include <QApplication>
#include <QFont>

void applyShaderLabTheme(QApplication& app)
{
    app.setStyle("Fusion");

    QFont font = app.font();
    font.setPointSize(10);
    app.setFont(font);

    app.setStyleSheet(R"(
        QMainWindow, QWidget { background-color: #15171c; color: #d8dce5; }
        QMenuBar { background-color: #111318; border-bottom: 1px solid #292d36; padding: 2px; }
        QMenuBar::item { padding: 6px 10px; background: transparent; }
        QMenuBar::item:selected { background-color: #262a33; border-radius: 4px; }
        QMenu { background-color: #1b1e25; border: 1px solid #303540; padding: 5px; }
        QMenu::item { padding: 7px 28px 7px 12px; border-radius: 4px; }
        QMenu::item:selected { background-color: #2a2f3a; }
        QToolBar { background-color: #111318; border: none; border-bottom: 1px solid #292d36; spacing: 5px; padding: 6px 8px; }
        QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 5px; padding: 5px; }
        QToolButton:hover { background-color: #252932; border-color: #343a46; }
        QToolButton:checked { background-color: #303644; }
        QDockWidget { color: #aeb5c2; font-weight: 600; }
        QDockWidget::title { background-color: #191c22; border-bottom: 1px solid #292d36; padding: 9px 10px; text-align: left; }
        QTreeWidget { background-color: #17191f; border: none; outline: none; padding: 4px; }
        QTreeWidget::item { min-height: 27px; border-radius: 4px; }
        QTreeWidget::item:hover { background-color: #22262e; }
        QTreeWidget::item:selected { background-color: #293142; }
        QHeaderView::section { background-color: #17191f; color: #777f8e; border: none; border-bottom: 1px solid #292d36; font-weight: 600; padding: 8px; }
        QPlainTextEdit { background-color: #101216; color: #aeb6c5; border: none; padding: 8px; font-family: monospace; selection-background-color: #39445a; }
        QLabel { background: transparent; }
        QLineEdit, QSpinBox, QDoubleSpinBox { background-color: #20232a; border: 1px solid #343943; border-radius: 5px; padding: 5px 7px; min-height: 22px; }
        QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover { border-color: #4c5361; }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color: #7184aa; }
        QPushButton { background-color: #252932; border: 1px solid #383e49; border-radius: 5px; padding: 6px 10px; }
        QPushButton:hover { background-color: #303540; }
        QStatusBar { background-color: #111318; border-top: 1px solid #292d36; color: #868e9e; }
        QStatusBar QLabel { padding: 0 8px; }
        QLabel#shaderName { color: #d9dde6; font-weight: 600; }
        QLabel#autoReloadEnabled { color: #8ac995; }
        QSplitter::handle { background-color: #292d36; }
        QScrollBar:vertical { width: 10px; background: #15171c; }
        QScrollBar::handle:vertical { background: #383d48; border-radius: 5px; min-height: 30px; }
        QScrollBar::handle:vertical:hover { background: #4a505c; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
    )");
}

#include <QApplication>
#include <QFont>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("影视");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("DoubanQt");

    QFont font = app.font();
    font.setFamily("Microsoft YaHei UI");
    font.setPixelSize(14);
    app.setFont(font);

    app.setStyleSheet(R"(
        QToolTip {
            background: #1A1A2E;
            color: #FFFFFF;
            border: none;
            padding: 8px 14px;
            border-radius: 6px;
            font-size: 12px;
        }
        QScrollBar:vertical {
            width: 6px;
            background: transparent;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: rgba(0, 0, 0, 0.12);
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(0, 0, 0, 0.22);
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollBar:horizontal {
            height: 6px;
            background: transparent;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: rgba(0, 0, 0, 0.12);
            border-radius: 3px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: rgba(0, 0, 0, 0.22);
        }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QScrollBar::add-page:horizontal,
        QScrollBar::sub-page:horizontal {
            background: transparent;
        }
    )");

    MainWindow window;
    window.show();

    return app.exec();
}

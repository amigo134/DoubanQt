#pragma once
#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QtMath>

class LoadingWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoadingWidget(QWidget* parent = nullptr)
        : QWidget(parent), m_angle(0)
    {
        setFixedHeight(36);
        m_timer.setInterval(60);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_angle = (m_angle + 30) % 360;
            update();
        });
        m_timer.start();
    }

    ~LoadingWidget()
    {
        m_timer.stop();
    }

    void setWidth(int w)
    {
        setFixedWidth(w);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int dotCount = 8;
        int radius = 6;
        int dotSize = 4;
        int cx = width() / 2 - 40;
        int cy = height() / 2;

        for (int i = 0; i < dotCount; ++i) {
            qreal a = qDegreesToRadians(static_cast<qreal>(m_angle + i * (360 / dotCount)));
            int x = static_cast<int>(cx + radius * qCos(a));
            int y = static_cast<int>(cy + radius * qSin(a));
            int alpha = 255 - i * 28;
            painter.setBrush(QColor(0, 181, 29, alpha));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(x - dotSize / 2, y - dotSize / 2, dotSize, dotSize);
        }

        painter.setPen(QColor(153, 153, 153));
        QFont f = painter.font();
        f.setPixelSize(12);
        painter.setFont(f);
        painter.drawText(QRect(cx + 16, 0, width() - cx - 16, height()),
                         Qt::AlignVCenter | Qt::AlignLeft, QString::fromUtf8("加载中..."));
    }

private:
    QTimer m_timer;
    int m_angle;
};

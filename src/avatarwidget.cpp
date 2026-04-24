#include "avatarwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

AvatarWidget::AvatarWidget(const QString& text, const QColor& color, int size, QWidget* parent)
    : QWidget(parent)
    , m_text(text)
    , m_color(color)
    , m_size(size)
{
    setFixedSize(m_size, m_size);
}

void AvatarWidget::setPixmap(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    m_usePixmap = !pixmap.isNull();
    update();
}

void AvatarWidget::setText(const QString& text)
{
    m_text = text;
    m_usePixmap = false;
    update();
}

void AvatarWidget::setColor(const QColor& color)
{
    m_color = color;
    update();
}

QSize AvatarWidget::sizeHint() const
{
    return QSize(m_size, m_size);
}

QSize AvatarWidget::minimumSizeHint() const
{
    return QSize(m_size, m_size);
}

void AvatarWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addEllipse(0, 0, m_size, m_size);
    painter.setClipPath(path);

    if (m_usePixmap && !m_pixmap.isNull()) {
        QPixmap scaled = m_pixmap.scaled(m_size, m_size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        painter.drawPixmap(0, 0, m_size, m_size, scaled);
    } else {
        painter.setBrush(m_color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, m_size, m_size);

        QString initial = m_text.isEmpty() ? QString("?") : QString(m_text.at(0)).toUpper();
        QFont font;
        font.setPixelSize(m_size * 0.45);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(QRect(0, 0, m_size, m_size), Qt::AlignCenter, initial);
    }
}

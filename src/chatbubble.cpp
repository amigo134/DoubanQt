#include "chatbubble.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QApplication>

ChatBubble::ChatBubble(const QString& text, bool isOwn, QWidget* parent)
    : QWidget(parent)
    , m_isOwn(isOwn)
{
    m_label = new QLabel(text, this);
    m_label->setWordWrap(true);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    if (m_isOwn) {
        m_label->setStyleSheet("QLabel { color: white; font-size: 14px; padding: 0px; background: transparent; }");
    } else {
        m_label->setStyleSheet("QLabel { color: #333333; font-size: 14px; padding: 0px; background: transparent; }");
    }

    calcSize();
}

void ChatBubble::calcSize()
{
    QFontMetrics fm(m_label->font());
    int textWidth = fm.horizontalAdvance(m_label->text());
    int singleLineHeight = fm.height();

    int availWidth = m_maxWidth - m_padding * 2;
    int labelX = m_isOwn ? m_padding : m_padding + m_arrowWidth;

    if (textWidth <= availWidth) {
        m_label->setGeometry(labelX, m_padding, textWidth, singleLineHeight);
        setFixedSize(textWidth + m_padding * 2 + m_arrowWidth, singleLineHeight + m_padding * 2);
    } else {
        QRect textRect = fm.boundingRect(QRect(0, 0, availWidth, 0), Qt::TextWordWrap, m_label->text());
        int w = availWidth;
        int h = textRect.height();
        m_label->setGeometry(labelX, m_padding, w, h);
        setFixedSize(w + m_padding * 2 + m_arrowWidth, h + m_padding * 2);
    }
}

int ChatBubble::arrowOffset() const
{
    return m_arrowWidth + m_padding;
}

QSize ChatBubble::sizeHint() const
{
    return size();
}

void ChatBubble::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bubbleColor = m_isOwn ? QColor("#95EC69") : QColor("#FFFFFF");
    QColor borderColor = m_isOwn ? QColor("#89D960") : QColor("#E8E8EC");

    int bubbleX = m_isOwn ? 0 : m_arrowWidth;
    int bubbleW = width() - m_arrowWidth;
    int bubbleH = height();

    QPainterPath bubblePath;
    bubblePath.addRoundedRect(bubbleX, 0, bubbleW, bubbleH, m_radius, m_radius);

    if (m_isOwn) {
        QPainterPath arrowPath;
        int arrowY = m_padding + 10;
        arrowPath.moveTo(bubbleX + bubbleW - 1, arrowY);
        arrowPath.lineTo(bubbleX + bubbleW + m_arrowWidth, arrowY + 6);
        arrowPath.lineTo(bubbleX + bubbleW - 1, arrowY + 12);
        bubblePath.addPath(arrowPath);
    } else {
        QPainterPath arrowPath;
        int arrowY = m_padding + 10;
        arrowPath.moveTo(m_arrowWidth, arrowY);
        arrowPath.lineTo(0, arrowY + 6);
        arrowPath.lineTo(m_arrowWidth, arrowY + 12);
        bubblePath.addPath(arrowPath);
    }

    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(bubbleColor);
    painter.drawPath(bubblePath);
}

void ChatBubble::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

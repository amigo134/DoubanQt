#include "chatmessagedelegate.h"
#include "chatmodel.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QApplication>

ChatMessageDelegate::ChatMessageDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void ChatMessageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    int itemType = index.data(ChatMessageModel::ItemTypeRole).toInt();
    if (itemType == static_cast<int>(ChatItemType::TimeSeparator)) {
        paintTimeSeparator(painter, option, index.data(ChatMessageModel::TimeTextRole).toString());
    } else {
        paintMessage(painter, option, index);
    }

    painter->restore();
}

QSize ChatMessageDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    int itemType = index.data(ChatMessageModel::ItemTypeRole).toInt();
    if (itemType == static_cast<int>(ChatItemType::TimeSeparator)) {
        return QSize(option.rect.width(), 36);
    }
    return messageSizeHint(option, index);
}

void ChatMessageDelegate::paintTimeSeparator(QPainter* painter,
                                             const QStyleOptionViewItem& option,
                                             const QString& timeText) const
{
    QRect rect = option.rect;

    QFont font;
    font.setPixelSize(11);
    painter->setFont(font);
    QFontMetrics fm(font);
    int textW = fm.horizontalAdvance(timeText);
    int textH = fm.height();
    int labelW = textW + 20;
    int labelH = textH + 4;

    QRect labelRect(rect.x() + (rect.width() - labelW) / 2,
                    rect.y() + (rect.height() - labelH) / 2,
                    labelW, labelH);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#E8E8EC"));
    painter->drawRoundedRect(labelRect, 4, 4);

    painter->setPen(QColor("#999999"));
    painter->drawText(labelRect, Qt::AlignCenter, timeText);
}

void ChatMessageDelegate::paintMessage(QPainter* painter,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const
{
    QRect rect = option.rect;
    bool isOwn = index.data(ChatMessageModel::IsOwnRole).toBool();
    QString from = index.data(ChatMessageModel::FromRole).toString();
    QString content = index.data(ChatMessageModel::ContentRole).toString();

    int avatarX = isOwn ? rect.right() - ITEM_H_MARGIN - AVATAR_SIZE
                        : rect.left() + ITEM_H_MARGIN;
    int avatarY = rect.top() + ITEM_V_MARGIN + NAME_LABEL_HEIGHT;

    int contentW = rect.width() - 2 * ITEM_H_MARGIN - AVATAR_SIZE - AVATAR_MSG_GAP;

    QFont bubbleFont;
    bubbleFont.setPixelSize(14);
    QFontMetrics fm(bubbleFont);

    QRect bubbleRect = calcBubbleRect(fm, content, contentW, isOwn);
    bubbleRect.moveTop(avatarY);

    if (isOwn) {
        bubbleRect.moveRight(avatarX - AVATAR_MSG_GAP);
    } else {
        bubbleRect.moveLeft(avatarX + AVATAR_SIZE + AVATAR_MSG_GAP);
    }

    QFont nameFont;
    nameFont.setPixelSize(11);
    painter->setFont(nameFont);
    painter->setPen(QColor("#999999"));

    if (isOwn) {
        painter->drawText(QRect(bubbleRect.x(), rect.top() + ITEM_V_MARGIN, bubbleRect.width() - ARROW_WIDTH, NAME_LABEL_HEIGHT),
                          Qt::AlignRight, from);
    } else {
        painter->drawText(QRect(bubbleRect.x() + ARROW_WIDTH, rect.top() + ITEM_V_MARGIN, bubbleRect.width() - ARROW_WIDTH, NAME_LABEL_HEIGHT),
                          Qt::AlignLeft, from);
    }

    drawAvatar(painter, QRect(avatarX, avatarY, AVATAR_SIZE, AVATAR_SIZE), from, isOwn);

    drawBubble(painter, bubbleRect, isOwn);

    QRect textRect = bubbleRect.adjusted(
        isOwn ? BUBBLE_PADDING : BUBBLE_PADDING + ARROW_WIDTH,
        BUBBLE_PADDING,
        isOwn ? -BUBBLE_PADDING - ARROW_WIDTH : -BUBBLE_PADDING,
        -BUBBLE_PADDING);

    painter->setFont(bubbleFont);
    painter->setPen(isOwn ? QColor("white") : QColor("#333333"));
    painter->drawText(textRect, Qt::TextWordWrap, content);
}

void ChatMessageDelegate::drawAvatar(QPainter* painter, const QRect& rect,
                                     const QString& name, bool isOwn) const
{
    QColor color = isOwn ? QColor("#00B51D") : QColor("#3498DB");

    QPainterPath path;
    path.addEllipse(rect);
    painter->setClipPath(path);

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(rect);

    QString initial = name.isEmpty() ? QString("?") : QString(name.at(0)).toUpper();
    QFont font;
    font.setPixelSize(rect.width() * 0.45);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(rect, Qt::AlignCenter, initial);

    painter->setClipping(false);
}

void ChatMessageDelegate::drawBubble(QPainter* painter, const QRect& bubbleRect,
                                     bool isOwn) const
{
    QColor bubbleColor = isOwn ? QColor("#95EC69") : QColor("#FFFFFF");
    QColor borderColor = isOwn ? QColor("#89D960") : QColor("#E8E8EC");

    int drawX = isOwn ? bubbleRect.x() : bubbleRect.x() + ARROW_WIDTH;
    int drawW = bubbleRect.width() - ARROW_WIDTH;
    int drawH = bubbleRect.height();

    QPainterPath bubblePath;
    bubblePath.addRoundedRect(drawX, bubbleRect.y(), drawW, drawH, BUBBLE_RADIUS, BUBBLE_RADIUS);

    if (isOwn) {
        QPainterPath arrowPath;
        int arrowY = bubbleRect.y() + BUBBLE_PADDING + 10;
        arrowPath.moveTo(drawX + drawW - 1, arrowY);
        arrowPath.lineTo(drawX + drawW + ARROW_WIDTH, arrowY + 6);
        arrowPath.lineTo(drawX + drawW - 1, arrowY + 12);
        bubblePath.addPath(arrowPath);
    } else {
        QPainterPath arrowPath;
        int arrowY = bubbleRect.y() + BUBBLE_PADDING + 10;
        arrowPath.moveTo(bubbleRect.x() + ARROW_WIDTH, arrowY);
        arrowPath.lineTo(bubbleRect.x(), arrowY + 6);
        arrowPath.lineTo(bubbleRect.x() + ARROW_WIDTH, arrowY + 12);
        bubblePath.addPath(arrowPath);
    }

    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(bubbleColor);
    painter->drawPath(bubblePath);
}

QRect ChatMessageDelegate::calcBubbleRect(const QFontMetrics& fm,
                                          const QString& content,
                                          int availWidth, bool isOwn) const
{
    int maxTextW = qMin(BUBBLE_MAX_WIDTH, availWidth) - BUBBLE_PADDING * 2 - ARROW_WIDTH;
    if (maxTextW < 40) maxTextW = 40;

    int textW = fm.horizontalAdvance(content);
    int singleLineH = fm.height();

    if (textW <= maxTextW) {
        int bubbleW = textW + BUBBLE_PADDING * 2 + ARROW_WIDTH;
        int bubbleH = singleLineH + BUBBLE_PADDING * 2;
        return QRect(0, 0, bubbleW, bubbleH);
    } else {
        QRect textRect = fm.boundingRect(QRect(0, 0, maxTextW, 0), Qt::TextWordWrap, content);
        int bubbleW = maxTextW + BUBBLE_PADDING * 2 + ARROW_WIDTH;
        int bubbleH = textRect.height() + BUBBLE_PADDING * 2;
        return QRect(0, 0, bubbleW, bubbleH);
    }
}

QSize ChatMessageDelegate::messageSizeHint(const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
{
    QString content = index.data(ChatMessageModel::ContentRole).toString();
    bool isOwn = index.data(ChatMessageModel::IsOwnRole).toBool();

    QFont bubbleFont;
    bubbleFont.setPixelSize(14);
    QFontMetrics fm(bubbleFont);

    int viewW = option.rect.width();
    if (viewW <= 0) viewW = 600;
    int contentW = viewW - 2 * ITEM_H_MARGIN - AVATAR_SIZE - AVATAR_MSG_GAP;

    QRect bubbleRect = calcBubbleRect(fm, content, contentW, isOwn);

    int totalH = ITEM_V_MARGIN + NAME_LABEL_HEIGHT + bubbleRect.height() + ITEM_V_MARGIN;
    return QSize(viewW, totalH);
}

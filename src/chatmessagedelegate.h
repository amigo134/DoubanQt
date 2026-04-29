#pragma once
#include <QStyledItemDelegate>
#include <QFontMetrics>

class ChatMessageDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ChatMessageDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    void paintTimeSeparator(QPainter* painter, const QStyleOptionViewItem& option,
                            const QString& timeText) const;
    void paintMessage(QPainter* painter, const QStyleOptionViewItem& option,
                      const QModelIndex& index) const;
    void drawAvatar(QPainter* painter, const QRect& rect, const QString& name,
                    int userId, bool isOwn) const;
    void drawBubble(QPainter* painter, const QRect& bubbleRect, bool isOwn) const;
    QRect calcBubbleRect(const QFontMetrics& fm, const QString& content,
                         int availWidth, bool isOwn) const;
    QSize messageSizeHint(const QStyleOptionViewItem& option,
                          const QModelIndex& index) const;

    static constexpr int AVATAR_SIZE = 40;
    static constexpr int AVATAR_MSG_GAP = 8;
    static constexpr int BUBBLE_PADDING = 10;
    static constexpr int BUBBLE_RADIUS = 8;
    static constexpr int ARROW_WIDTH = 8;
    static constexpr int BUBBLE_MAX_WIDTH = 400;
    static constexpr int ITEM_H_MARGIN = 12;
    static constexpr int ITEM_V_MARGIN = 6;
    static constexpr int NAME_LABEL_HEIGHT = 16;
};

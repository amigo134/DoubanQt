#include "chatmessagewidget.h"
#include <QDate>

ChatMessageWidget::ChatMessageWidget(const QString& username, const QString& content,
                                     const QString& time, bool isOwn,
                                     QWidget* parent)
    : QWidget(parent)
    , m_fromUser(username)
    , m_isOwn(isOwn)
    , m_time(time)
{
    buildUI(username, content, time, isOwn);
}

QString ChatMessageWidget::fromUser() const
{
    return m_fromUser;
}

bool ChatMessageWidget::isOwnMessage() const
{
    return m_isOwn;
}

QString ChatMessageWidget::messageTime() const
{
    return m_time;
}

QString ChatMessageWidget::formatTime(const QString& time)
{
    QDateTime dt = QDateTime::fromString(time, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(time, "hh:mm");
        if (dt.isValid()) return time;
        return time;
    }

    QDate msgDate = dt.date();
    QDate today = QDate::currentDate();

    if (msgDate == today) {
        return dt.toString("hh:mm");
    } else if (msgDate.year() == today.year() && msgDate.month() == today.month()) {
        return dt.toString("dd日 hh:mm");
    } else if (msgDate.year() == today.year()) {
        return dt.toString("MM-dd hh:mm");
    } else {
        return dt.toString("yyyy-MM-dd hh:mm");
    }
}

void ChatMessageWidget::buildUI(const QString& username, const QString& content,
                                const QString& time, bool isOwn)
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 6, 12, 1);
    mainLayout->setSpacing(0);

    QColor avatarColor = isOwn ? QColor("#00B51D") : QColor("#3498DB");

    auto* bubble = new ChatBubble(content, isOwn, this);
    int bubbleWidth = bubble->width();
    int offset = bubble->arrowOffset();

    auto* contentWidget = new QWidget();
    contentWidget->setStyleSheet("background: transparent;");
    contentWidget->setFixedWidth(bubbleWidth);

    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(2);

    auto* nameLabel = new QLabel(username);
    nameLabel->setStyleSheet("color: #999; font-size: 11px; background: transparent;");
    if (isOwn) {
        nameLabel->setAlignment(Qt::AlignRight);
        nameLabel->setContentsMargins(0, 0, offset, 0);
    } else {
        nameLabel->setAlignment(Qt::AlignLeft);
        nameLabel->setContentsMargins(offset, 0, 0, 0);
    }
    contentLayout->addWidget(nameLabel);

    contentLayout->addWidget(bubble);

    if (isOwn) {
        auto* avatar = new AvatarWidget(username, avatarColor, 40, this);
        mainLayout->addStretch(1);
        mainLayout->addWidget(contentWidget, 0, Qt::AlignRight);
        mainLayout->addSpacing(8);
        mainLayout->addWidget(avatar, 0, Qt::AlignVCenter);
    } else {
        auto* avatar = new AvatarWidget(username, avatarColor, 40, this);
        mainLayout->addWidget(avatar, 0, Qt::AlignVCenter);
        mainLayout->addSpacing(8);
        mainLayout->addWidget(contentWidget, 0, Qt::AlignLeft);
        mainLayout->addStretch(1);
    }

    setStyleSheet("background: transparent;");
}

QWidget* ChatMessageWidget::createTimeLabel(const QString& time)
{
    QString displayText = formatTime(time);

    auto* label = new QLabel(displayText);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(R"(
        QLabel {
            color: #999;
            font-size: 11px;
            background: #E8E8EC;
            border-radius: 4px;
            padding: 2px 10px;
        }
    )");
    label->setContentsMargins(0, 6, 0, 6);
    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();
    container->setStyleSheet("background: transparent;");
    return container;
}

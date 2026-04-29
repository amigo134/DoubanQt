#include "chatmodel.h"
#include <QDate>

ChatMessageModel::ChatMessageModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ChatMessageModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant ChatMessageModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const ChatItem& item = m_items[index.row()];

    switch (role) {
    case ItemTypeRole:
        return static_cast<int>(item.type);
    case FromRole:
        return item.msg.from;
    case FromIdRole:
        return item.msg.fromId;
    case ToIdRole:
        return item.msg.toId;
    case ContentRole:
        return item.msg.content;
    case TimeRole:
        return item.msg.time;
    case IsOwnRole:
        return item.msg.isOwn;
    case MsgIdRole:
        return item.msg.id;
    case TimeTextRole:
        return item.timeText;
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const
{
    return {
        {ItemTypeRole, "itemType"},
        {FromRole, "from"},
        {FromIdRole, "fromId"},
        {ToIdRole, "toId"},
        {ContentRole, "content"},
        {TimeRole, "time"},
        {IsOwnRole, "isOwn"},
        {MsgIdRole, "msgId"},
        {TimeTextRole, "timeText"},
    };
}

void ChatMessageModel::appendMessage(const ChatMsg& msg)
{
    QString displayTime = formatTime(msg.time);

    int insertRow = m_items.size();

    if (shouldInsertTimeSeparator(msg.time)) {
        beginInsertRows(QModelIndex(), insertRow, insertRow);
        m_items.append(ChatItem::makeSeparator(displayTime));
        endInsertRows();
        insertRow++;
    }

    beginInsertRows(QModelIndex(), insertRow, insertRow);
    m_items.append(ChatItem::makeMessage(msg));
    endInsertRows();
}

void ChatMessageModel::prependMessages(const QList<ChatMsg>& msgs)
{
    if (msgs.isEmpty()) return;

    int insertCount = 0;
    QList<ChatItem> newItems;

    for (int i = 0; i < msgs.size(); ++i) {
        const ChatMsg& msg = msgs[i];
        QString displayTime = formatTime(msg.time);

        bool needSeparator = true;
        if (i > 0) {
            needSeparator = shouldShowTime(msgs[i - 1].time, msg.time);
        } else if (!m_items.isEmpty()) {
            QString firstExistingTime;
            for (const auto& existing : m_items) {
                if (existing.type == ChatItemType::Message) {
                    firstExistingTime = existing.msg.time;
                    break;
                }
            }
            if (!firstExistingTime.isEmpty()) {
                needSeparator = shouldShowTime(msg.time, firstExistingTime);
            }
        }

        if (needSeparator) {
            newItems.append(ChatItem::makeSeparator(displayTime));
        }
        newItems.append(ChatItem::makeMessage(msg));
    }

    if (!m_items.isEmpty() && m_items.first().type == ChatItemType::TimeSeparator) {
        bool stillNeeded = false;
        if (m_items.size() > 1 && m_items[1].type == ChatItemType::Message) {
            stillNeeded = shouldShowTime(
                newItems.last().type == ChatItemType::Message
                    ? newItems.last().msg.time
                    : QString(),
                m_items[1].msg.time);
        }
        if (!stillNeeded) {
            beginRemoveRows(QModelIndex(), 0, 0);
            m_items.removeFirst();
            endRemoveRows();
        }
    }

    insertCount = newItems.size();
    beginInsertRows(QModelIndex(), 0, insertCount - 1);
    for (int i = newItems.size() - 1; i >= 0; --i) {
        m_items.prepend(newItems[i]);
    }
    endInsertRows();
}

void ChatMessageModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

bool ChatMessageModel::isEmpty() const
{
    return m_items.isEmpty();
}

int ChatMessageModel::oldestMsgId() const
{
    for (const auto& item : m_items) {
        if (item.type == ChatItemType::Message && item.msg.id > 0) {
            return item.msg.id;
        }
    }
    return 0;
}

int ChatMessageModel::newestMsgId() const
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i].type == ChatItemType::Message && m_items[i].msg.id > 0) {
            return m_items[i].msg.id;
        }
    }
    return 0;
}

bool ChatMessageModel::shouldShowTime(const QString& prevTime, const QString& curTime) const
{
    if (prevTime.isEmpty() || curTime.isEmpty()) return true;

    QDateTime prevDt = QDateTime::fromString(prevTime, Qt::ISODate);
    QDateTime curDt = QDateTime::fromString(curTime, Qt::ISODate);
    if (!prevDt.isValid() || !curDt.isValid()) return true;

    return qAbs(prevDt.secsTo(curDt)) >= 300;
}

bool ChatMessageModel::shouldInsertTimeSeparator(const QString& msgTime) const
{
    if (m_items.isEmpty()) return true;

    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i].type == ChatItemType::Message) {
            return shouldShowTime(m_items[i].msg.time, msgTime);
        }
    }
    return true;
}

QString ChatMessageModel::formatTime(const QString& time) const
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

#pragma once
#include <QString>
#include <QList>
#include <QAbstractListModel>
#include <QDateTime>

struct FriendInfo {
    int userId = 0;
    QString username;
    bool online = false;
};

struct ChatMsg {
    int id = 0;
    int fromId = 0;
    int toId = 0;
    QString from;
    QString to;
    QString content;
    QString time;
    bool isOwn = false;
};

enum class ChatItemType { TimeSeparator, Message };

struct ChatItem {
    ChatItemType type;
    QString timeText;
    ChatMsg msg;

    static ChatItem makeSeparator(const QString& time) {
        ChatItem item;
        item.type = ChatItemType::TimeSeparator;
        item.timeText = time;
        return item;
    }

    static ChatItem makeMessage(const ChatMsg& m) {
        ChatItem item;
        item.type = ChatItemType::Message;
        item.msg = m;
        item.timeText = m.time;
        return item;
    }
};

class ChatMessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        ItemTypeRole = Qt::UserRole + 1,
        FromRole,
        FromIdRole,
        ToIdRole,
        ContentRole,
        TimeRole,
        IsOwnRole,
        MsgIdRole,
        TimeTextRole,
    };

    explicit ChatMessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void appendMessage(const ChatMsg& msg);
    void prependMessages(const QList<ChatMsg>& msgs);
    void clear();
    bool isEmpty() const;
    int oldestMsgId() const;
    int newestMsgId() const;

private:
    bool shouldShowTime(const QString& prevTime, const QString& curTime) const;
    bool shouldInsertTimeSeparator(const QString& msgTime) const;
    QString formatTime(const QString& time) const;

    QList<ChatItem> m_items;
};

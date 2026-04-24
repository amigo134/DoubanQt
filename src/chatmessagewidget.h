#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>
#include "avatarwidget.h"
#include "chatbubble.h"

class ChatMessageWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatMessageWidget(const QString& username, const QString& content,
                               const QString& time, bool isOwn,
                               QWidget* parent = nullptr);

    static QWidget* createTimeLabel(const QString& time);
    static QString formatTime(const QString& time);

    QString fromUser() const;
    bool isOwnMessage() const;
    QString messageTime() const;

private:
    void buildUI(const QString& username, const QString& content,
                 const QString& time, bool isOwn);

    QString m_fromUser;
    bool m_isOwn;
    QString m_time;
};

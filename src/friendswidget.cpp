#include "friendswidget.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QFrame>

FriendsWidget::FriendsWidget(ChatManager* chatMgr, QWidget* parent)
    : QWidget(parent)
    , m_chatMgr(chatMgr)
{
    buildUI();

    connect(m_chatMgr, &ChatManager::loginResult,
            this, &FriendsWidget::onLoginResult);
    connect(m_chatMgr, &ChatManager::friendListReceived,
            this, &FriendsWidget::onFriendListReceived);
    connect(m_chatMgr, &ChatManager::friendRequestReceived,
            this, &FriendsWidget::onFriendRequestReceived);
    connect(m_chatMgr, &ChatManager::addFriendResult,
            this, &FriendsWidget::onAddFriendResult);
    connect(m_chatMgr, &ChatManager::friendAccepted,
            this, &FriendsWidget::onFriendAccepted);
    connect(m_chatMgr, &ChatManager::messageReceived,
            this, &FriendsWidget::onMessageReceived);
    connect(m_chatMgr, &ChatManager::onlineStatusChanged,
            this, &FriendsWidget::onOnlineStatusChanged);
    connect(m_chatMgr, &ChatManager::disconnected,
            this, &FriendsWidget::onChatDisconnected);
}

void FriendsWidget::buildUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* leftPanel = new QWidget();
    leftPanel->setFixedWidth(260);
    leftPanel->setStyleSheet("background: white; border-right: 1px solid #E8E8EC;");

    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_statusLabel = new QLabel("未连接");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedHeight(36);
    m_statusLabel->setStyleSheet("background: #F5F6F8; color: #999; font-size: 12px; border-bottom: 1px solid #E8E8EC;");
    leftLayout->addWidget(m_statusLabel);

    auto* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(10, 8, 10, 8);
    btnRow->setSpacing(8);

    m_addFriendBtn = new QPushButton("添加好友");
    m_addFriendBtn->setStyleSheet(R"(
        QPushButton { background: #00B51D; color: white; border: none; border-radius: 6px; padding: 6px 12px; font-size: 12px; font-weight: bold; }
        QPushButton:hover { background: #009A18; }
    )");
    connect(m_addFriendBtn, &QPushButton::clicked, this, &FriendsWidget::onAddFriendClicked);
    btnRow->addWidget(m_addFriendBtn);

    m_friendReqBtn = new QPushButton("好友请求");
    m_friendReqBtn->setStyleSheet(R"(
        QPushButton { background: #F5A623; color: white; border: none; border-radius: 6px; padding: 6px 12px; font-size: 12px; font-weight: bold; }
        QPushButton:hover { background: #E09500; }
    )");
    connect(m_friendReqBtn, &QPushButton::clicked, this, &FriendsWidget::onFriendRequestClicked);
    btnRow->addWidget(m_friendReqBtn);

    leftLayout->addLayout(btnRow);

    m_friendList = new QListWidget();
    m_friendList->setStyleSheet(R"(
        QListWidget { border: none; font-size: 14px; }
        QListWidget::item { padding: 12px 16px; border-bottom: 1px solid #F0F0F2; }
        QListWidget::item:selected { background: #E8F5E9; color: #00B51D; }
        QListWidget::item:hover { background: #F5F6F8; }
    )");
    connect(m_friendList, &QListWidget::itemClicked,
            this, &FriendsWidget::onFriendItemClicked);
    leftLayout->addWidget(m_friendList);

    mainLayout->addWidget(leftPanel);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_chatArea = new QWidget();
    auto* chatLayout = new QVBoxLayout(m_chatArea);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(0);

    m_chatTitle = new QLabel();
    m_chatTitle->setFixedHeight(48);
    m_chatTitle->setStyleSheet("background: white; border-bottom: 1px solid #E8E8EC; font-size: 15px; font-weight: bold; color: #333; padding-left: 20px;");
    chatLayout->addWidget(m_chatTitle);

    m_chatHistory = new QTextEdit();
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setStyleSheet(R"(
        QTextEdit { background: #F5F6F8; border: none; font-size: 14px; padding: 12px; }
    )");
    chatLayout->addWidget(m_chatHistory);

    auto* inputRow = new QHBoxLayout();
    inputRow->setContentsMargins(12, 8, 12, 12);
    inputRow->setSpacing(8);

    m_msgInput = new QLineEdit();
    m_msgInput->setPlaceholderText("输入消息...");
    m_msgInput->setFixedHeight(40);
    m_msgInput->setStyleSheet(R"(
        QLineEdit { border: 1px solid #ddd; border-radius: 8px; padding: 0 14px; font-size: 14px; }
        QLineEdit:focus { border-color: #00B51D; }
    )");
    connect(m_msgInput, &QLineEdit::returnPressed, this, &FriendsWidget::onSendClicked);
    inputRow->addWidget(m_msgInput);

    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setFixedSize(70, 40);
    m_sendBtn->setStyleSheet(R"(
        QPushButton { background: #00B51D; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: bold; }
        QPushButton:hover { background: #009A18; }
    )");
    connect(m_sendBtn, &QPushButton::clicked, this, &FriendsWidget::onSendClicked);
    inputRow->addWidget(m_sendBtn);

    chatLayout->addLayout(inputRow);

    m_placeholder = new QWidget();
    m_placeholder->setStyleSheet("background: #F5F6F8;");
    auto* phLayout = new QVBoxLayout(m_placeholder);
    phLayout->setAlignment(Qt::AlignCenter);
    auto* phLabel = new QLabel("选择一个好友开始聊天");
    phLabel->setAlignment(Qt::AlignCenter);
    phLabel->setStyleSheet("color: #BBB; font-size: 16px;");
    phLayout->addWidget(phLabel);

    rightLayout->addWidget(m_placeholder);
    rightLayout->addWidget(m_chatArea);

    m_chatArea->setVisible(false);
    m_placeholder->setVisible(true);

    mainLayout->addWidget(rightPanel, 1);
}

void FriendsWidget::refreshFriendList()
{
    if (m_chatMgr->isConnected()) {
        m_chatMgr->requestFriendList();
    }
}

void FriendsWidget::onFriendItemClicked(QListWidgetItem* item)
{
    QString username = item->data(Qt::UserRole).toString();
    openChatWith(username);
}

void FriendsWidget::onSendClicked()
{
    if (m_currentChatUser.isEmpty()) return;
    QString content = m_msgInput->text().trimmed();
    if (content.isEmpty()) return;

    m_chatMgr->sendMessage(m_currentChatUser, content);
    QString time = QDateTime::currentDateTime().toString("hh:mm");
    appendChatMessage("我", content, time, true);

    ChatMsg msg;
    msg.from = "me";
    msg.to = m_currentChatUser;
    msg.content = content;
    msg.time = time;
    msg.isOwn = true;
    m_chatMessages[m_currentChatUser].append(msg);

    m_msgInput->clear();
}

void FriendsWidget::onAddFriendClicked()
{
    bool ok = false;
    QString username = QInputDialog::getText(this, "添加好友", "输入用户名:", QLineEdit::Normal, "", &ok);
    if (ok && !username.trimmed().isEmpty()) {
        m_chatMgr->sendAddFriend(username.trimmed());
    }
}

void FriendsWidget::onFriendRequestClicked()
{
    if (m_pendingRequests.isEmpty()) {
        QMessageBox::information(this, "好友请求", "暂无好友请求");
        return;
    }

    QStringList items = m_pendingRequests;
    bool ok = false;
    QString selected = QInputDialog::getItem(this, "好友请求", "选择要处理的请求:", items, 0, false, &ok);
    if (!ok || selected.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "好友请求", QString("是否同意 %1 的好友请求？").arg(selected),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_chatMgr->acceptFriend(selected);
    } else {
        m_chatMgr->rejectFriend(selected);
    }
}

void FriendsWidget::onLoginResult(bool success)
{
    if (success) {
        m_statusLabel->setText("已连接");
        m_statusLabel->setStyleSheet("background: #E8F5E9; color: #00B51D; font-size: 12px; border-bottom: 1px solid #E8E8EC;");
        emit connectionStatusChanged(true);
    } else {
        m_statusLabel->setText("连接失败");
        m_statusLabel->setStyleSheet("background: #FDE8E8; color: #e74c3c; font-size: 12px; border-bottom: 1px solid #E8E8EC;");
        emit connectionStatusChanged(false);
    }
}

void FriendsWidget::onFriendListReceived(const QList<FriendInfo>& friends)
{
    m_friends = friends;
    updateFriendListUI();
}

void FriendsWidget::onFriendRequestReceived(const QString& from)
{
    if (!m_pendingRequests.contains(from)) {
        m_pendingRequests.append(from);
        updateRequestBadge();
    }
}

void FriendsWidget::onAddFriendResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "添加好友", "好友请求已发送");
    } else {
        QMessageBox::warning(this, "添加好友", message);
    }
}

void FriendsWidget::onFriendAccepted(const QString& username)
{
    m_pendingRequests.removeAll(username);
    updateRequestBadge();
    m_chatMgr->requestFriendList();
}

void FriendsWidget::onMessageReceived(const QString& from, const QString& content, const QString& time)
{
    ChatMsg msg;
    msg.from = from;
    msg.content = content;
    msg.time = time;
    msg.isOwn = false;
    m_chatMessages[from].append(msg);

    if (from == m_currentChatUser) {
        appendChatMessage(from, content, time, false);
    }
}

void FriendsWidget::onOnlineStatusChanged(const QString& username, bool online)
{
    for (auto& f : m_friends) {
        if (f.username == username) {
            f.online = online;
            break;
        }
    }
    updateFriendListUI();

    if (username == m_currentChatUser) {
        m_chatTitle->setText(username + (online ? "  🟢 在线" : "  ⚪ 离线"));
    }
}

void FriendsWidget::onChatDisconnected()
{
    m_statusLabel->setText("已断开");
    m_statusLabel->setStyleSheet("background: #FDE8E8; color: #e74c3c; font-size: 12px; border-bottom: 1px solid #E8E8EC;");
    emit connectionStatusChanged(false);

    for (auto& f : m_friends) {
        f.online = false;
    }
    updateFriendListUI();
}

void FriendsWidget::updateFriendListUI()
{
    m_friendList->clear();

    std::sort(m_friends.begin(), m_friends.end(),
              [](const FriendInfo& a, const FriendInfo& b) {
                  if (a.online != b.online) return a.online > b.online;
                  return a.username < b.username;
              });

    for (const auto& f : m_friends) {
        auto* item = new QListWidgetItem(
            QString("%1 %2").arg(f.online ? "🟢" : "⚪").arg(f.username));
        item->setData(Qt::UserRole, f.username);
        m_friendList->addItem(item);
    }
}

void FriendsWidget::openChatWith(const QString& username)
{
    m_currentChatUser = username;
    m_chatArea->setVisible(true);
    m_placeholder->setVisible(false);

    bool online = false;
    for (const auto& f : m_friends) {
        if (f.username == username) { online = f.online; break; }
    }
    m_chatTitle->setText(username + (online ? "  🟢 在线" : "  ⚪ 离线"));

    m_chatHistory->clear();
    for (const auto& msg : m_chatMessages[username]) {
        appendChatMessage(msg.isOwn ? "我" : msg.from, msg.content, msg.time, msg.isOwn);
    }

    m_msgInput->setFocus();
}

void FriendsWidget::appendChatMessage(const QString& from, const QString& content, const QString& time, bool isOwn)
{
    QString html;
    if (isOwn) {
        html = QString(R"(<div style="text-align: right; margin: 6px 0;">
            <span style="color: #999; font-size: 11px;">%1</span><br>
            <span style="background: #00B51D; color: white; padding: 6px 12px; border-radius: 12px; display: inline-block; max-width: 70%%; word-break: break-word;">%2</span>
        </div>)").arg(time, content.toHtmlEscaped());
    } else {
        html = QString(R"(<div style="text-align: left; margin: 6px 0;">
            <span style="color: #00B51D; font-weight: bold; font-size: 12px;">%1</span>
            <span style="color: #999; font-size: 11px;"> %2</span><br>
            <span style="background: white; color: #333; padding: 6px 12px; border-radius: 12px; display: inline-block; max-width: 70%%; word-break: break-word; border: 1px solid #E8E8EC;">%3</span>
        </div>)").arg(from.toHtmlEscaped(), time, content.toHtmlEscaped());
    }
    m_chatHistory->append(html);
    m_chatHistory->verticalScrollBar()->setValue(m_chatHistory->verticalScrollBar()->maximum());
}

void FriendsWidget::showPlaceholder()
{
    m_chatArea->setVisible(false);
    m_placeholder->setVisible(true);
    m_currentChatUser.clear();
}

void FriendsWidget::updateRequestBadge()
{
    if (m_pendingRequests.isEmpty()) {
        m_friendReqBtn->setText("好友请求");
    } else {
        m_friendReqBtn->setText(QString("好友请求 (%1)").arg(m_pendingRequests.size()));
    }
}

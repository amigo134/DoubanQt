#include "friendswidget.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QFrame>
#include <QResizeEvent>

FriendsWidget::FriendsWidget(ChatManager* chatMgr, ServerApiClient* serverApi, QWidget* parent)
    : QWidget(parent)
    , m_chatMgr(chatMgr)
    , m_serverApi(serverApi)
    , m_chatModel(nullptr)
    , m_chatDelegate(nullptr)
    , m_loadingIndicator(nullptr)
{
    buildUI();

    connect(m_chatMgr, &ChatManager::loginResult,
            this, &FriendsWidget::onLoginResult);
    connect(m_chatMgr, &ChatManager::friendRequestReceived,
            this, &FriendsWidget::onFriendRequestReceived);
    connect(m_chatMgr, &ChatManager::friendAccepted,
            this, &FriendsWidget::onFriendAccepted);
    connect(m_chatMgr, &ChatManager::messageReceived,
            this, &FriendsWidget::onMessageReceived);
    connect(m_chatMgr, &ChatManager::messageSent,
            this, &FriendsWidget::onMessageSent);
    connect(m_chatMgr, &ChatManager::onlineStatusChanged,
            this, &FriendsWidget::onOnlineStatusChanged);
    connect(m_chatMgr, &ChatManager::disconnected,
            this, &FriendsWidget::onChatDisconnected);

    // HTTP-based operations via ServerApiClient
    connect(m_serverApi, &ServerApiClient::friendListReceived,
            this, &FriendsWidget::onFriendListReceived);
    connect(m_serverApi, &ServerApiClient::addFriendResult,
            this, &FriendsWidget::onAddFriendResult);
    connect(m_serverApi, &ServerApiClient::chatHistoryReceived,
            this, &FriendsWidget::onChatHistoryReceived);
}

bool FriendsWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_chatListView->viewport() && event->type() == QEvent::Resize) {
        if (m_loadingIndicator && m_loadingIndicator->isVisible()) {
            int w = m_chatListView->viewport()->width();
            m_loadingIndicator->move(0, 0);
            m_loadingIndicator->setWidth(w);
            m_loadingIndicator->setFixedWidth(w);
        }
    }
    return QWidget::eventFilter(watched, event);
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

    m_chatListView = new QListView();
    m_chatListView->setStyleSheet(R"(
        QListView { background: #F5F6F8; border: none; }
        QScrollBar:vertical { width: 6px; background: transparent; }
        QScrollBar::handle:vertical { background: #CCC; border-radius: 3px; min-height: 30px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");
    m_chatListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatListView->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_chatListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    m_chatDelegate = new ChatMessageDelegate(m_chatListView);
    m_chatListView->setItemDelegate(m_chatDelegate);

    m_chatListView->viewport()->installEventFilter(this);

    connect(m_chatListView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &FriendsWidget::onChatScrollBarValueChanged);

    chatLayout->addWidget(m_chatListView);

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
    int uid = m_chatMgr->serverUserId();
    if (uid > 0) {
        m_serverApi->getFriendList(uid);
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
    m_msgInput->clear();
}

void FriendsWidget::onAddFriendClicked()
{
    bool ok = false;
    QString username = QInputDialog::getText(this, "添加好友", "输入用户名:", QLineEdit::Normal, "", &ok);
    if (ok && !username.trimmed().isEmpty()) {
        int uid = m_chatMgr->serverUserId();
        if (uid > 0) m_serverApi->addFriend(uid, username.trimmed());
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

    int uid = m_chatMgr->serverUserId();
    if (reply == QMessageBox::Yes) {
        if (uid > 0) m_serverApi->acceptFriend(uid, selected);
    } else {
        if (uid > 0) m_serverApi->rejectFriend(uid, selected);
    }
}

void FriendsWidget::onLoginResult(bool success)
{
    if (success) {
        m_statusLabel->setText("已连接");
        m_statusLabel->setStyleSheet("background: #E8F5E9; color: #00B51D; font-size: 12px; border-bottom: 1px solid #E8E8EC;");
        emit connectionStatusChanged(true);
        refreshFriendList();
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

void FriendsWidget::onFriendRequestReceived(const QString& from, int fromId)
{
    Q_UNUSED(fromId);
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

void FriendsWidget::onFriendAccepted(const QString& username, int userId)
{
    Q_UNUSED(userId);
    m_pendingRequests.removeAll(username);
    updateRequestBadge();
    int uid = m_chatMgr->serverUserId();
    if (uid > 0) m_serverApi->getFriendList(uid);
}

void FriendsWidget::onMessageReceived(const QString& from, const QString& content, const QString& time, int msgId, int fromId)
{
    ChatMsg msg;
    msg.id = msgId;
    msg.fromId = fromId;
    msg.from = from;
    msg.content = content;
    msg.time = time;
    msg.isOwn = false;

    if (fromId > 0 && fromId == m_currentChatUserId && m_chatModel) {
        m_chatModel->appendMessage(msg);
        scrollToBottom();
    } else {
        if (!m_chatModels.contains(from)) {
            m_chatModels[from] = new ChatMessageModel(this);
        }
        m_chatModels[from]->appendMessage(msg);
    }
}

void FriendsWidget::onMessageSent(const QString& to, const QString& content, const QString& time, int msgId, int toId)
{
    ChatMsg msg;
    msg.id = msgId;
    msg.toId = toId;
    msg.from = m_chatMgr->currentUsername();
    msg.to = to;
    msg.content = content;
    msg.time = time;
    msg.isOwn = true;

    if (toId > 0 && toId == m_currentChatUserId && m_chatModel) {
        m_chatModel->appendMessage(msg);
        scrollToBottom();
    } else {
        if (!m_chatModels.contains(to)) {
            m_chatModels[to] = new ChatMessageModel(this);
        }
        m_chatModels[to]->appendMessage(msg);
    }
}

void FriendsWidget::onOnlineStatusChanged(const QString& username, bool online, int userId)
{
    for (auto& f : m_friends) {
        if (f.userId == userId) {
            f.online = online;
            break;
        }
    }
    updateFriendListUI();

    if (userId > 0 && userId == m_currentChatUserId) {
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

void FriendsWidget::onChatHistoryReceived(const QString& with, const QList<ChatMsg>& messages, bool hasMore)
{
    if (with != m_currentChatUser) {
        m_isLoadingHistory = false;
        return;
    }

    removeTopLoadingIndicator();

    m_hasMoreHistory = hasMore;
    m_hasMoreHistoryMap[with] = hasMore;

    if (messages.isEmpty()) {
        m_isLoadingHistory = false;
        m_isInitialLoad = false;
        return;
    }

    if (m_isInitialLoad) {
        m_chatListView->setUpdatesEnabled(false);

        m_chatModel->clear();

        for (const auto& msg : messages) {
            m_chatModel->appendMessage(msg);
        }

        m_historyLoadedUsers.insert(with);

        m_chatListView->doItemsLayout();

        m_chatListView->setUpdatesEnabled(true);

        m_isInitialLoad = false;
        m_isLoadingHistory = false;

        QMetaObject::invokeMethod(this, [this]() {
            QScrollBar* bar = m_chatListView->verticalScrollBar();
            bar->setValue(bar->maximum());
        }, Qt::QueuedConnection);
    } else {
        int oldRowCount = m_chatModel->rowCount();

        QModelIndex anchorIndex;
        for (int i = 0; i < oldRowCount; ++i) {
            QModelIndex idx = m_chatModel->index(i, 0);
            if (idx.data(ChatMessageModel::ItemTypeRole).toInt() == static_cast<int>(ChatItemType::Message)) {
                anchorIndex = idx;
                break;
            }
        }

        int anchorRow = anchorIndex.isValid() ? anchorIndex.row() : -1;

        m_chatListView->setUpdatesEnabled(false);

        m_chatModel->prependMessages(messages);

        m_chatListView->doItemsLayout();

        if (anchorRow >= 0) {
            int addedCount = m_chatModel->rowCount() - oldRowCount;
            int newAnchorRow = anchorRow + addedCount;
            QModelIndex newAnchorIndex = m_chatModel->index(newAnchorRow, 0);
            if (newAnchorIndex.isValid()) {
                m_chatListView->scrollTo(newAnchorIndex, QAbstractItemView::PositionAtTop);
            }
        } else {
            QScrollBar* bar = m_chatListView->verticalScrollBar();
            bar->setValue(bar->maximum());
        }

        m_chatListView->setUpdatesEnabled(true);

        m_isLoadingHistory = false;
    }
}

void FriendsWidget::onChatScrollBarValueChanged(int value)
{
    if (value > 0) return;
    if (m_currentChatUser.isEmpty()) return;
    if (m_isLoadingHistory) return;
    if (!m_hasMoreHistory) return;
    if (m_chatModel->isEmpty()) return;

    requestHistory();
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
        if (f.username == username) {
            online = f.online;
            m_currentChatUserId = f.userId;
            break;
        }
    }
    m_chatTitle->setText(username + (online ? "  🟢 在线" : "  ⚪ 离线"));

    if (m_chatModels.contains(username)) {
        m_chatModel = m_chatModels[username];
    } else {
        m_chatModel = new ChatMessageModel(this);
        m_chatModels[username] = m_chatModel;
    }
    m_chatListView->setModel(m_chatModel);

    m_isLoadingHistory = false;
    m_hasMoreHistory = m_hasMoreHistoryMap.value(username, true);

    if (!m_historyLoadedUsers.contains(username)) {
        m_isInitialLoad = true;
        m_isLoadingHistory = true;
        int uid = m_chatMgr->serverUserId();
        if (uid > 0) m_serverApi->getChatHistory(uid, username, 30, 0);
    } else {
        m_isInitialLoad = false;
        QMetaObject::invokeMethod(this, [this]() {
            QScrollBar* bar = m_chatListView->verticalScrollBar();
            bar->setValue(bar->maximum());
        }, Qt::QueuedConnection);
    }

    m_msgInput->setFocus();
}

void FriendsWidget::scrollToBottom()
{
    QMetaObject::invokeMethod(this, [this]() {
        QScrollBar* bar = m_chatListView->verticalScrollBar();
        bar->setValue(bar->maximum());
    }, Qt::QueuedConnection);
}

void FriendsWidget::requestHistory()
{
    if (!m_chatModel || m_chatModel->isEmpty()) return;

    m_isLoadingHistory = true;
    showTopLoadingIndicator();

    int oldestId = m_chatModel->oldestMsgId();
    int uid = m_chatMgr->serverUserId();
    if (uid > 0) m_serverApi->getChatHistory(uid, m_currentChatUser, 30, oldestId);
}

void FriendsWidget::showTopLoadingIndicator()
{
    if (m_loadingIndicator) return;

    m_loadingIndicator = new LoadingWidget(m_chatListView->viewport());
    m_loadingIndicator->setWidth(m_chatListView->viewport()->width());
    m_loadingIndicator->setGeometry(0, 0, m_chatListView->viewport()->width(), 44);
    m_loadingIndicator->show();
    m_loadingIndicator->raise();
}

void FriendsWidget::removeTopLoadingIndicator()
{
    if (m_loadingIndicator) {
        m_loadingIndicator->deleteLater();
        m_loadingIndicator = nullptr;
    }
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

#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("登录");
    setFixedSize(380, 460);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QLabel("影视");
    header->setAlignment(Qt::AlignCenter);
    header->setFixedHeight(80);
    header->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00B51D, stop:1 #00D4AA);
        color: white;
        font-size: 28px;
        font-weight: bold;
    )");
    root->addWidget(header);

    m_stack = new QStackedWidget();
    root->addWidget(m_stack);

    setupLoginPage();
    setupRegisterPage();

    m_msgLabel = new QLabel();
    m_msgLabel->setAlignment(Qt::AlignCenter);
    m_msgLabel->setFixedHeight(30);
    m_msgLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
    root->addWidget(m_msgLabel);
}

void LoginDialog::setupLoginPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 28, 36, 20);
    layout->setSpacing(14);

    auto* title = new QLabel("欢迎回来");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    layout->addWidget(title);

    m_loginUser = new QLineEdit();
    m_loginUser->setPlaceholderText("用户名");
    m_loginUser->setFixedHeight(42);
    m_loginUser->setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 0 14px;
            font-size: 14px;
        }
        QLineEdit:focus { border-color: #00B51D; }
    )");
    layout->addWidget(m_loginUser);

    m_loginPwd = new QLineEdit();
    m_loginPwd->setPlaceholderText("密码");
    m_loginPwd->setEchoMode(QLineEdit::Password);
    m_loginPwd->setFixedHeight(42);
    m_loginPwd->setStyleSheet(m_loginUser->styleSheet());
    layout->addWidget(m_loginPwd);

    auto* checkRow = new QHBoxLayout();
    checkRow->setSpacing(20);
    m_rememberCheck = new QCheckBox("记住密码");
    m_rememberCheck->setStyleSheet("QCheckBox { font-size: 13px; color: #666; } QCheckBox::indicator { width: 16px; height: 16px; }");
    m_autoLoginCheck = new QCheckBox("自动登录");
    m_autoLoginCheck->setStyleSheet(m_rememberCheck->styleSheet());
    connect(m_rememberCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            m_autoLoginCheck->setChecked(false);
        }
    });
    connect(m_autoLoginCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            m_rememberCheck->setChecked(true);
        }
    });
    checkRow->addWidget(m_rememberCheck);
    checkRow->addWidget(m_autoLoginCheck);
    checkRow->addStretch();
    layout->addLayout(checkRow);

    auto* loginBtn = new QPushButton("登 录");
    loginBtn->setFixedHeight(44);
    loginBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00B51D, stop:1 #00D4AA);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton:hover { background: #009A18; }
    )");
    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::doLogin);
    layout->addWidget(loginBtn);

    layout->addSpacing(8);

    auto* switchRow = new QHBoxLayout();
    switchRow->setSpacing(4);
    auto* hint = new QLabel("还没有账号？");
    hint->setStyleSheet("color: #999; font-size: 13px;");
    switchRow->addWidget(hint);
    auto* regLink = new QLabel("注册");
    regLink->setStyleSheet("color: #00B51D; font-size: 13px; font-weight: bold;");
    regLink->setCursor(Qt::PointingHandCursor);
    connect(regLink, &QLabel::linkActivated, this, [this]() {
        m_msgLabel->clear();
        m_stack->setCurrentIndex(1);
        setWindowTitle("注册");
    });
    regLink->installEventFilter(this);
    switchRow->addWidget(regLink);
    switchRow->addStretch();
    layout->addLayout(switchRow);

    layout->addStretch();
    m_stack->addWidget(page);
}

void LoginDialog::setupRegisterPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 28, 36, 20);
    layout->setSpacing(14);

    auto* title = new QLabel("创建账号");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    layout->addWidget(title);

    m_regUser = new QLineEdit();
    m_regUser->setPlaceholderText("用户名");
    m_regUser->setMaxLength(20);
    m_regUser->setFixedHeight(42);
    m_regUser->setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 0 14px;
            font-size: 14px;
        }
        QLineEdit:focus { border-color: #00B51D; }
    )");
    layout->addWidget(m_regUser);

    m_regPwd = new QLineEdit();
    m_regPwd->setPlaceholderText("密码");
    m_regPwd->setEchoMode(QLineEdit::Password);
    m_regPwd->setFixedHeight(42);
    m_regPwd->setStyleSheet(m_regUser->styleSheet());
    layout->addWidget(m_regPwd);

    m_regPwd2 = new QLineEdit();
    m_regPwd2->setPlaceholderText("确认密码");
    m_regPwd2->setEchoMode(QLineEdit::Password);
    m_regPwd2->setFixedHeight(42);
    m_regPwd2->setStyleSheet(m_regUser->styleSheet());
    layout->addWidget(m_regPwd2);

    auto* regBtn = new QPushButton("注 册");
    regBtn->setFixedHeight(44);
    regBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00B51D, stop:1 #00D4AA);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton:hover { background: #009A18; }
    )");
    connect(regBtn, &QPushButton::clicked, this, &LoginDialog::doRegister);
    layout->addWidget(regBtn);

    layout->addSpacing(8);

    auto* switchRow = new QHBoxLayout();
    switchRow->setSpacing(4);
    auto* hint = new QLabel("已有账号？");
    hint->setStyleSheet("color: #999; font-size: 13px;");
    switchRow->addWidget(hint);
    auto* loginLink = new QLabel("登录");
    loginLink->setStyleSheet("color: #00B51D; font-size: 13px; font-weight: bold;");
    loginLink->setCursor(Qt::PointingHandCursor);
    connect(loginLink, &QLabel::linkActivated, this, [this]() {
        m_msgLabel->clear();
        m_stack->setCurrentIndex(0);
        setWindowTitle("登录");
    });
    loginLink->installEventFilter(this);
    switchRow->addWidget(loginLink);
    switchRow->addStretch();
    layout->addLayout(switchRow);

    layout->addStretch();
    m_stack->addWidget(page);
}

void LoginDialog::doLogin()
{
    QString user = m_loginUser->text().trimmed();
    QString pwd = m_loginPwd->text();
    if (user.isEmpty() || pwd.isEmpty()) {
        m_msgLabel->setText("请输入用户名和密码");
        return;
    }
    m_msgLabel->clear();
    accept();
}

void LoginDialog::doRegister()
{
    QString user = m_regUser->text().trimmed();
    QString pwd = m_regPwd->text();
    QString pwd2 = m_regPwd2->text();
    if (user.isEmpty() || pwd.isEmpty()) {
        m_msgLabel->setText("请输入用户名和密码");
        return;
    }
    if (pwd != pwd2) {
        m_msgLabel->setText("两次密码不一致");
        return;
    }
    if (pwd.length() < 4) {
        m_msgLabel->setText("密码至少4位");
        return;
    }
    m_msgLabel->clear();
    accept();
}

QString LoginDialog::getUsername() const
{
    if (m_stack->currentIndex() == 0)
        return m_loginUser->text().trimmed();
    return m_regUser->text().trimmed();
}

QString LoginDialog::getPassword() const
{
    if (m_stack->currentIndex() == 0)
        return m_loginPwd->text();
    return m_regPwd->text();
}

bool LoginDialog::isRegister() const
{
    return m_stack->currentIndex() == 1;
}

void LoginDialog::switchToRegister()
{
    m_stack->setCurrentIndex(1);
    setWindowTitle("注册");
}

bool LoginDialog::rememberPassword() const
{
    return m_rememberCheck->isChecked();
}

bool LoginDialog::autoLogin() const
{
    return m_autoLoginCheck->isChecked();
}

void LoginDialog::setLoginInfo(const QString& user, const QString& pwd, bool remember, bool autoLogin)
{
    m_loginUser->setText(user);
    m_loginPwd->setText(pwd);
    m_rememberCheck->setChecked(remember);
    m_autoLoginCheck->setChecked(autoLogin);
}

bool LoginDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* label = qobject_cast<QLabel*>(watched);
        if (label) {
            QString text = label->text();
            if (text == "注册") {
                m_msgLabel->clear();
                m_stack->setCurrentIndex(1);
                setWindowTitle("注册");
            } else if (text == "登录") {
                m_msgLabel->clear();
                m_stack->setCurrentIndex(0);
                setWindowTitle("登录");
            }
            return true;
        }
    }
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (m_stack->currentIndex() == 0) doLogin();
            else doRegister();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

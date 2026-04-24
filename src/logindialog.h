#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QStackedWidget>
#include <QLabel>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);

    QString getUsername() const;
    QString getPassword() const;
    bool isRegister() const;
    bool rememberPassword() const;
    bool autoLogin() const;
    void switchToRegister();
    void setLoginInfo(const QString& user, const QString& pwd, bool remember, bool autoLogin);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QStackedWidget* m_stack;
    QLineEdit* m_loginUser;
    QLineEdit* m_loginPwd;
    QLineEdit* m_regUser;
    QLineEdit* m_regPwd;
    QLineEdit* m_regPwd2;
    QCheckBox* m_rememberCheck;
    QCheckBox* m_autoLoginCheck;
    QLabel* m_msgLabel;

    void setupLoginPage();
    void setupRegisterPage();
    void doLogin();
    void doRegister();
};

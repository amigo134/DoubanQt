#include "profileeditdialog.h"
#include <QKeyEvent>

ProfileEditDialog::ProfileEditDialog(const QString& name, const QString& bio, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("编辑个人信息");
    setFixedSize(400, 280);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(16);

    auto* nameLabel = new QLabel("昵称");
    nameLabel->setStyleSheet("font-size: 13px; color: #555; font-weight: bold;");
    root->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(name);
    m_nameEdit->setMaxLength(20);
    m_nameEdit->setPlaceholderText("输入你的昵称");
    m_nameEdit->setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #DDD;
            border-radius: 8px;
            padding: 10px 14px;
            font-size: 14px;
            background: #FAFAFA;
        }
        QLineEdit:focus {
            border-color: #00B51D;
            background: white;
        }
    )");
    root->addWidget(m_nameEdit);

    auto* bioLabel = new QLabel("个性签名");
    bioLabel->setStyleSheet("font-size: 13px; color: #555; font-weight: bold;");
    root->addWidget(bioLabel);

    m_bioEdit = new QTextEdit(bio);
    m_bioEdit->setMaximumHeight(80);
    m_bioEdit->setPlaceholderText("写点什么介绍自己吧...");
    m_bioEdit->setStyleSheet(R"(
        QTextEdit {
            border: 1px solid #DDD;
            border-radius: 8px;
            padding: 8px 14px;
            font-size: 14px;
            background: #FAFAFA;
        }
        QTextEdit:focus {
            border-color: #00B51D;
            background: white;
        }
    )");
    root->addWidget(m_bioEdit);

    root->addStretch();

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);
    btnRow->addStretch();

    auto* cancelBtn = new QPushButton("取消");
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background: #F5F5F5;
            border: 1px solid #DDD;
            border-radius: 8px;
            padding: 9px 28px;
            font-size: 14px;
            color: #666;
        }
        QPushButton:hover { background: #EEE; }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto* saveBtn = new QPushButton("保存");
    saveBtn->setStyleSheet(R"(
        QPushButton {
            background: #00B51D;
            border: none;
            border-radius: 8px;
            padding: 9px 28px;
            font-size: 14px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover { background: #009A18; }
    )");
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) return;
        accept();
    });
    btnRow->addWidget(saveBtn);

    root->addLayout(btnRow);

    m_nameEdit->installEventFilter(this);
    m_bioEdit->installEventFilter(this);
}

bool ProfileEditDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (watched == m_nameEdit || watched == m_bioEdit) {
                if (!m_nameEdit->text().trimmed().isEmpty()) {
                    accept();
                }
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

QString ProfileEditDialog::getName() const
{
    return m_nameEdit->text().trimmed();
}

QString ProfileEditDialog::getBio() const
{
    return m_bioEdit->toPlainText().trimmed();
}

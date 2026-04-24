#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class ProfileEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProfileEditDialog(const QString& name, const QString& bio, QWidget* parent = nullptr);

    QString getName() const;
    QString getBio() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QLineEdit* m_nameEdit;
    QTextEdit* m_bioEdit;
};

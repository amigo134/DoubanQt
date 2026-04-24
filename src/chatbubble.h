#pragma once
#include <QWidget>
#include <QLabel>

class ChatBubble : public QWidget {
    Q_OBJECT
public:
    explicit ChatBubble(const QString& text, bool isOwn, QWidget* parent = nullptr);

    int arrowOffset() const;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void calcSize();

    QLabel* m_label;
    bool m_isOwn;
    int m_arrowWidth = 8;
    int m_padding = 10;
    int m_radius = 8;
    int m_maxWidth = 400;
};

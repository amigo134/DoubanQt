#pragma once
#include <QWidget>
#include <QPixmap>

class AvatarWidget : public QWidget {
    Q_OBJECT
public:
    explicit AvatarWidget(const QString& text, const QColor& color, int size = 40, QWidget* parent = nullptr);

    void setPixmap(const QPixmap& pixmap);
    void setText(const QString& text);
    void setColor(const QColor& color);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_text;
    QColor m_color;
    int m_size;
    QPixmap m_pixmap;
    bool m_usePixmap = false;
};

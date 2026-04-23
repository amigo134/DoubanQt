#pragma once
#include <QWidget>

class RatingWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double rating READ rating WRITE setRating NOTIFY ratingChanged)
public:
    explicit RatingWidget(QWidget* parent = nullptr);

    double rating() const { return m_rating; }
    void setRating(double rating);
    void setReadOnly(bool readOnly);
    void setStarSize(int size);
    void setMaxRating(int max);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void ratingChanged(double rating);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    double m_rating = 0.0;
    double m_hoverRating = 0.0;
    bool m_readOnly = false;
    int m_starSize = 24;
    int m_maxRating = 10;
    int m_starCount = 5;

    double ratingFromPos(int x) const;
};

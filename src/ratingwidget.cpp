#include "ratingwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPolygonF>
#include <cmath>

RatingWidget::RatingWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    updateGeometry();
}

void RatingWidget::setRating(double rating)
{
    m_rating = qBound(0.0, rating, (double)m_maxRating);
    update();
    emit ratingChanged(m_rating);
}

void RatingWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    setMouseTracking(!readOnly);
}

void RatingWidget::setStarSize(int size)
{
    m_starSize = size;
    setFixedSize(m_starCount * (size + 4), size + 4);
}

void RatingWidget::setMaxRating(int max)
{
    m_maxRating = max;
}

double RatingWidget::ratingFromPos(int x) const
{
    double starWidth = m_starSize + 4;
    double pos = x / starWidth;
    double rating = (pos + 0.5) * (m_maxRating / m_starCount);
    return qBound(0.0, rating, (double)m_maxRating);
}

void RatingWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_readOnly) return;
    m_hoverRating = ratingFromPos(event->pos().x());
    update();
}

void RatingWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_readOnly || event->button() != Qt::LeftButton) return;
    setRating(ratingFromPos(event->pos().x()));
}

void RatingWidget::leaveEvent(QEvent*)
{
    m_hoverRating = 0.0;
    update();
}

static QPolygonF starPolygon(double cx, double cy, double outerR, double innerR)
{
    QPolygonF poly;
    const int points = 5;
    for (int i = 0; i < points * 2; ++i) {
        double angle = (i * M_PI / points) - M_PI / 2;
        double r = (i % 2 == 0) ? outerR : innerR;
        poly << QPointF(cx + r * std::cos(angle), cy + r * std::sin(angle));
    }
    return poly;
}

void RatingWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    double displayRating = (m_hoverRating > 0 && !m_readOnly) ? m_hoverRating : m_rating;
    double starsToFill = displayRating / m_maxRating * m_starCount;

    int starSpacing = 4;
    double outerR = m_starSize / 2.0;
    double innerR = outerR * 0.4;

    for (int i = 0; i < m_starCount; ++i) {
        double cx = i * (m_starSize + starSpacing) + outerR + starSpacing / 2.0;
        double cy = height() / 2.0;

        QPolygonF star = starPolygon(cx, cy, outerR, innerR);

        double fill = qBound(0.0, starsToFill - i, 1.0);

        if (fill >= 1.0) {
            painter.setBrush(QColor(255, 165, 0));
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(star);
        } else if (fill > 0.0) {
            painter.setBrush(QColor(230, 230, 230));
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(star);

            painter.save();
            painter.setClipRect(QRectF(cx - outerR, 0, outerR * 2 * fill, height()));
            painter.setBrush(QColor(255, 165, 0));
            painter.drawPolygon(star);
            painter.restore();
        } else {
            painter.setBrush(QColor(230, 230, 230));
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(star);
        }
    }
}

QSize RatingWidget::minimumSizeHint() const
{
    return QSize(m_starCount * (m_starSize + 4), m_starSize + 4);
}

QSize RatingWidget::sizeHint() const
{
    return minimumSizeHint();
}

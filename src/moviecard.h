#pragma once
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include "moviemodel.h"

class MovieCard : public QFrame {
    Q_OBJECT
public:
    explicit MovieCard(const Movie& movie, QWidget* parent = nullptr);

    void setMovie(const Movie& movie);
    const Movie& movie() const { return m_movie; }

signals:
    void clicked(const Movie& movie);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void loadPoster(const QString& url);
    void updateUserStatus();

    Movie m_movie;
    QLabel* m_posterLabel;
    QLabel* m_titleLabel;
    QLabel* m_ratingLabel;
    QLabel* m_genreLabel;
    QLabel* m_statusLabel;
    QWidget* m_ratingBadge;
};

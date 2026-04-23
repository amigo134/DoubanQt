#pragma once
#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include "ratingwidget.h"
#include "moviemodel.h"

class ReviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReviewDialog(const Movie& movie, const UserReview& existingReview,
                          QWidget* parent = nullptr);

    double getRating() const;
    QString getReview() const;
    bool isWished() const;
    bool isWatched() const;

private:
    RatingWidget* m_ratingWidget;
    QTextEdit* m_reviewEdit;
    QLabel* m_ratingLabel;
    bool m_wished = false;
    bool m_watched = false;

    void updateRatingLabel(double rating);
};

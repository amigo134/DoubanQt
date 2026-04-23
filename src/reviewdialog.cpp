#include "reviewdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QCheckBox>
#include <cmath>

static const QStringList RATING_LABELS = {
    "", "很差", "较差", "还行", "推荐", "力荐",
    "很差", "较差", "还行", "推荐", "力荐"
};

ReviewDialog::ReviewDialog(const Movie& movie, const UserReview& existingReview, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("我的评价 - " + movie.getName());
    setMinimumWidth(500);
    setStyleSheet(R"(
        QDialog {
            background: #FFFFFF;
        }
        QLabel#titleLabel {
            font-size: 18px;
            font-weight: bold;
            color: #222;
        }
        QLabel#sectionLabel {
            font-size: 13px;
            color: #666;
            margin-top: 10px;
            font-weight: bold;
        }
        QTextEdit {
            border: 1px solid #DDD;
            border-radius: 8px;
            padding: 12px;
            font-size: 14px;
            background: #FAFAFA;
            color: #222;
        }
        QTextEdit:focus {
            border-color: #00B51D;
            background: white;
        }
        QPushButton#submitBtn {
            background: #00B51D;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 10px 36px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#submitBtn:hover {
            background: #009A18;
        }
        QPushButton#submitBtn:pressed {
            background: #008012;
        }
        QPushButton#cancelBtn {
            background: transparent;
            color: #888;
            border: 1px solid #DDD;
            border-radius: 6px;
            padding: 10px 28px;
            font-size: 14px;
        }
        QPushButton#cancelBtn:hover {
            border-color: #BBB;
            color: #555;
        }
        QCheckBox {
            font-size: 13px;
            color: #555;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 4px;
            border: 1px solid #CCC;
        }
        QCheckBox::indicator:checked {
            background: #00B51D;
            border-color: #00B51D;
        }
        QCheckBox::indicator:hover {
            border-color: #00B51D;
        }
    )");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(28, 28, 28, 28);

    auto* headerRow = new QHBoxLayout();
    auto* titleLabel = new QLabel(movie.getName());
    titleLabel->setObjectName("titleLabel");
    headerRow->addWidget(titleLabel);
    headerRow->addStretch();
    mainLayout->addLayout(headerRow);

    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #EEE;");
    mainLayout->addWidget(line);

    auto* ratingSection = new QLabel("⭐ 我的评分");
    ratingSection->setObjectName("sectionLabel");
    mainLayout->addWidget(ratingSection);

    auto* ratingLayout = new QHBoxLayout();
    m_ratingWidget = new RatingWidget(this);
    m_ratingWidget->setStarSize(32);
    m_ratingWidget->setRating(existingReview.rating);

    m_ratingLabel = new QLabel();
    m_ratingLabel->setStyleSheet("font-size: 14px; color: #00B51D; font-weight: bold; margin-left: 12px;");
    updateRatingLabel(existingReview.rating);

    ratingLayout->addWidget(m_ratingWidget);
    ratingLayout->addWidget(m_ratingLabel);
    ratingLayout->addStretch();
    mainLayout->addLayout(ratingLayout);

    connect(m_ratingWidget, &RatingWidget::ratingChanged, this, [this](double r) {
        updateRatingLabel(r);
    });

    auto* reviewSection = new QLabel("✏ 我的短评");
    reviewSection->setObjectName("sectionLabel");
    mainLayout->addWidget(reviewSection);

    m_reviewEdit = new QTextEdit(this);
    m_reviewEdit->setPlaceholderText("写下你的观后感... (最多500字)");
    m_reviewEdit->setPlainText(existingReview.content);
    m_reviewEdit->setMaximumHeight(130);
    mainLayout->addWidget(m_reviewEdit);

    auto* statusLayout = new QHBoxLayout();
    auto* wishedCheck = new QCheckBox("想看");
    auto* watchedCheck = new QCheckBox("看过");
    wishedCheck->setChecked(existingReview.isWished);
    watchedCheck->setChecked(existingReview.isWatched);
    m_wished = existingReview.isWished;
    m_watched = existingReview.isWatched;

    connect(wishedCheck, &QCheckBox::toggled, this, [this](bool v) { m_wished = v; });
    connect(watchedCheck, &QCheckBox::toggled, this, [this](bool v) { m_watched = v; });

    statusLayout->addWidget(wishedCheck);
    statusLayout->addWidget(watchedCheck);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);

    mainLayout->addSpacing(6);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("cancelBtn");
    auto* submitBtn = new QPushButton("保存评价");
    submitBtn->setObjectName("submitBtn");

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(submitBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addSpacing(10);
    btnLayout->addWidget(submitBtn);
    mainLayout->addLayout(btnLayout);
}

double ReviewDialog::getRating() const
{
    return m_ratingWidget->rating();
}

QString ReviewDialog::getReview() const
{
    return m_reviewEdit->toPlainText().left(500);
}

bool ReviewDialog::isWished() const { return m_wished; }
bool ReviewDialog::isWatched() const { return m_watched; }

void ReviewDialog::updateRatingLabel(double rating)
{
    if (rating <= 0) {
        m_ratingLabel->setText("还未评分");
        return;
    }
    int idx = qBound(1, (int)std::ceil(rating / 2.0), 5);
    m_ratingLabel->setText(QString("%1 (%2分)").arg(RATING_LABELS[idx]).arg(rating, 0, 'f', 1));
}

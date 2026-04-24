#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QTimer>
#include "moviemodel.h"
#include "databasemanager.h"

class ProfileWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProfileWidget(DatabaseManager* db, QWidget* parent = nullptr);

    void refresh();

signals:
    void movieClicked(const Movie& movie);
    void logoutRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUI();
    void loadData();
    void loadProfile();
    void loadAvatar(const QString& path);
    void saveProfile(const QString& name, const QString& bio);
    void rebuildWishRows();
    void rebuildWatchedRows();
    void rebuildReviewRows();
    int calcCols(int cardW) const;

    DatabaseManager* m_db;
    QTimer* m_resizeTimer;

    QLabel* m_avatarLabel;
    QLabel* m_nameLabel;
    QLabel* m_bioLabel;
    QLabel* m_statWatched;
    QLabel* m_statWished;
    QLabel* m_statReviews;
    QFrame* m_profileCard;

    QWidget* m_wishWrap;
    QVBoxLayout* m_wishOuterLayout;
    QLabel* m_wishEmptyLabel;
    QList<QFrame*> m_wishCards;
    QList<UserReview> m_wishData;
    int m_wishCols = 0;

    QWidget* m_watchedWrap;
    QVBoxLayout* m_watchedOuterLayout;
    QLabel* m_watchedEmptyLabel;
    QList<QFrame*> m_watchedCards;
    QList<UserReview> m_watchedData;
    int m_watchedCols = 0;

    QWidget* m_reviewWrap;
    QVBoxLayout* m_reviewOuterLayout;
    QLabel* m_reviewEmptyLabel;
    QList<QFrame*> m_reviewCards;
    QList<UserReview> m_reviewData;
    QList<UserReview> m_reviewFiltered;

    static constexpr int CARD_W = 110;
    static constexpr int CARD_H = 160;
    static constexpr int MARGIN = 20;
    static constexpr int SPACING = 12;
};

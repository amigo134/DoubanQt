#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QResizeEvent>
#include <QTimer>
#include "moviemodel.h"
#include "databasemanager.h"

class HomeWidget : public QWidget {
    Q_OBJECT
public:
    explicit HomeWidget(DatabaseManager* db, QWidget* parent = nullptr);

    void refresh();
    void setTop250Data(const QList<Movie>& movies);

signals:
    void movieSearchRequested(const QString& query);
    void movieClicked(const Movie& movie);
    void top250Requested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUI();
    void rebuildHotSearchRows();
    void rebuildMyListRows();
    void rebuildTop250Rows();
    int calcHotSearchCols() const;
    int calcMyListCols() const;
    int calcTop250Cols() const;

    DatabaseManager* m_db;
    QTimer* m_resizeTimer;

    QWidget* m_hotSearchWrap;
    QVBoxLayout* m_hotSearchOuterLayout;
    QList<QPushButton*> m_hotBtns;
    int m_hotCols = 0;

    QWidget* m_myListWrap;
    QVBoxLayout* m_myListOuterLayout;
    QLabel* m_emptyLabel;
    QList<QFrame*> m_myCards;
    int m_myCols = 0;

    QWidget* m_top250Wrap;
    QVBoxLayout* m_top250OuterLayout;
    QLabel* m_top250EmptyLabel;
    QList<QFrame*> m_top250Cards;
    QList<Movie> m_top250Data;
    int m_top250Cols = 0;

    QList<UserReview> m_watchedData;

    static const QStringList HOT_SEARCHES;
    static constexpr int MY_CARD_W = 120;
    static constexpr int MY_CARD_H = 150;
    static constexpr int TOP250_CARD_W = 110;
    static constexpr int TOP250_CARD_H = 170;
    static constexpr int MARGIN = 20;
    static constexpr int SPACING = 12;
};

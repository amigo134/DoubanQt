#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QTimer>
#include "moviemodel.h"
#include "databasemanager.h"

class HomeWidget : public QWidget {
    Q_OBJECT
public:
    explicit HomeWidget(DatabaseManager* db, QWidget* parent = nullptr);

    void refresh();

signals:
    void movieSearchRequested(const QString& query);
    void movieClicked(const Movie& movie);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUI();
    void rearrangeHotSearch();
    void rearrangeMyList();
    int calculateHotSearchColumns() const;
    int calculateMyListColumns() const;

    DatabaseManager* m_db;
    QTimer* m_resizeTimer;

    QWidget* m_hotSearchWrap;
    QGridLayout* m_hotSearchGrid;
    QList<QPushButton*> m_hotSearchBtns;
    int m_hotSearchCols = 0;

    QWidget* m_myListWrap;
    QGridLayout* m_myListGrid;
    QLabel* m_emptyLabel;
    QList<QFrame*> m_myCards;
    int m_myListCols = 0;

    QList<UserReview> m_watchedData;

    static const QStringList HOT_SEARCHES;
    static constexpr int MY_CARD_WIDTH = 110;
    static constexpr int MY_CARD_HEIGHT = 140;
    static constexpr int MY_CARD_SPACING = 14;
    static constexpr int MY_MARGIN = 20;
    static constexpr int HOT_MARGIN = 20;
    static constexpr int HOT_SPACING = 12;
};

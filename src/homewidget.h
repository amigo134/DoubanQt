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
    void rebuildHotSearchRows();
    void rebuildMyListRows();
    int calcHotSearchCols() const;
    int calcMyListCols() const;

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

    QList<UserReview> m_watchedData;

    static const QStringList HOT_SEARCHES;
    static constexpr int MY_CARD_W = 110;
    static constexpr int MY_CARD_H = 140;
    static constexpr int MARGIN = 20;
    static constexpr int SPACING = 12;
};

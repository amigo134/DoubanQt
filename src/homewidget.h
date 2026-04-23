#pragma once
#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QResizeEvent>
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
    void buildHotSearchSection(QVBoxLayout* layout);
    void buildMyListSection(QVBoxLayout* layout);
    void refreshMyList();
    void rearrangeHotSearch();
    void rearrangeMyList();
    int calculateHotSearchColumns() const;
    int calculateMyListColumns() const;

    DatabaseManager* m_db;
    QWidget* m_myListWidget;
    QGridLayout* m_myListGrid;
    QLabel* m_emptyLabel;
    QWidget* m_hotSearchWrap;
    QGridLayout* m_hotSearchGrid;

    QList<UserReview> m_watchedData;

    static const QStringList HOT_SEARCHES;
    static constexpr int MY_CARD_WIDTH = 110;
    static constexpr int MY_CARD_SPACING = 14;
    static constexpr int MY_MARGIN = 18;
    static constexpr int HOT_MARGIN = 18;
    static constexpr int HOT_SPACING = 10;
};

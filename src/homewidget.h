#pragma once
#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
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

private:
    void buildHotSearchSection(QVBoxLayout* layout);
    void buildMyListSection(QVBoxLayout* layout);
    void refreshMyList();

    DatabaseManager* m_db;
    QWidget* m_myListWidget;
    QGridLayout* m_myListGrid;
    QLabel* m_emptyLabel;

    static const QStringList HOT_SEARCHES;
};

#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>
#include "apimanager.h"
#include "databasemanager.h"
#include "chatmanager.h"
#include "searchresultwidget.h"
#include "moviedetailwidget.h"
#include "homewidget.h"
#include "profilewidget.h"
#include "friendswidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onSearch();
    void onSearchResultReady(const SearchResult& result);
    void onLoadMore(int skip);
    void onMovieClicked(const Movie& movie);
    void onMovieDetailReady(const Movie& movie);
    void onBackFromDetail();
    void onNavClicked(int index);
    void onNetworkBusy(bool busy);
    void onLogout();

private:
    void buildUI();
    void performSearch(const QString& query, int skip = 0);
    void updateSearchBoxWidth();
    bool showLogin();

    ApiManager* m_api;
    DatabaseManager* m_db;
    ChatManager* m_chatMgr;

    QLineEdit* m_searchEdit;
    QStackedWidget* m_stackedWidget;
    SearchResultWidget* m_searchResultWidget;
    MovieDetailWidget* m_detailWidget;
    HomeWidget* m_homeWidget;
    ProfileWidget* m_profileWidget;
    FriendsWidget* m_friendsWidget;

    QLabel* m_loadingLabel;
    QPushButton* m_navHome;
    QPushButton* m_navSearch;
    QPushButton* m_navChat;
    QPushButton* m_navProfile;
    QFrame* m_searchContainer;

    QString m_currentQuery;
    int m_prevPage = HOME;
    enum Page { HOME = 0, SEARCH = 1, DETAIL = 2, CHAT = 3, PROFILE = 4 };
};

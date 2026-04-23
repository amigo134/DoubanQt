#pragma once
#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QResizeEvent>
#include <QLayout>
#include <QStyle>
#include "moviemodel.h"
#include "databasemanager.h"

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent, int margin = -1, int hspacing = -1, int vspacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;
    QSize sizeHint() const override;
    QLayoutItem* takeAt(int index) override;

private:
    int doLayout(const QRect& rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem*> m_itemList;
    int m_hSpace;
    int m_vSpace;
};

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
    void rearrangeMyList();
    int calculateMyListColumns() const;

    DatabaseManager* m_db;
    QWidget* m_myListWidget;
    QGridLayout* m_myListGrid;
    QLabel* m_emptyLabel;
    QWidget* m_hotSearchWrap;
    FlowLayout* m_hotSearchFlow;

    QList<UserReview> m_watchedData;

    static const QStringList HOT_SEARCHES;
    static constexpr int MY_CARD_WIDTH = 110;
    static constexpr int MY_CARD_SPACING = 14;
    static constexpr int MY_MARGIN = 18;
};

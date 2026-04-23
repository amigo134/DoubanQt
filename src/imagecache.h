#pragma once
#include <QObject>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QHash>

class ImageCache : public QObject {
    Q_OBJECT
public:
    static ImageCache& instance();

    void loadImage(const QString& url, QObject* receiver,
                   std::function<void(const QPixmap&)> callback);

private:
    explicit ImageCache(QObject* parent = nullptr);
    QNetworkAccessManager* m_nam;
    QHash<QString, QPixmap> m_cache;
};

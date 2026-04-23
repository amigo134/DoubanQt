#include "imagecache.h"
#include <QNetworkRequest>
#include <QNetworkReply>

ImageCache& ImageCache::instance()
{
    static ImageCache inst;
    return inst;
}

ImageCache::ImageCache(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void ImageCache::loadImage(const QString& url, QObject* receiver,
                            std::function<void(const QPixmap&)> callback)
{
    if (url.isEmpty()) return;

    if (m_cache.contains(url)) {
        callback(m_cache[url]);
        return;
    }

    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 DoubanQt/1.0");
    QNetworkReply* reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, receiver, [this, reply, url, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll())) {
            m_cache[url] = pixmap;
            callback(pixmap);
        }
    });
}

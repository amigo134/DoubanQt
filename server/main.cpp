#include <QCoreApplication>
#include "chatserver.h"
#include "httpserver.h"
#include "serverdb.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("DoubanChatServer");

    ServerDb db(5);
    if (!db.initialize()) {
        qWarning() << "DB init failed";
        return 1;
    }

    quint16 wsPort = 8765;
    quint16 httpPort = 8766;
    if (argc > 1) {
        bool ok = false;
        quint16 p = QString(argv[1]).toUShort(&ok);
        if (ok) wsPort = p;
    }

    ChatServer chatServer(&db, wsPort);
    HttpServer httpServer(&db, httpPort);

    qDebug() << "Server started — WebSocket:" << wsPort << "HTTP:" << httpPort;

    return app.exec();
}

#include <QCoreApplication>
#include "chatserver.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("DoubanChatServer");

    quint16 port = 8765;
    if (argc > 1) {
        bool ok = false;
        quint16 p = QString(argv[1]).toUShort(&ok);
        if (ok) port = p;
    }

    ChatServer server(port);
    qDebug() << "Chat server started on port" << port;

    return app.exec();
}

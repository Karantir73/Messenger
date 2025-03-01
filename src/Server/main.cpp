#include <QApplication>
#include <exception>
#include "servercontroller.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);

    qInfo() << "--- start server ---";
    ServerController server;
    server.startServer();

    return QApplication::exec();
}

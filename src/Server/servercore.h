#ifndef SERVER_CORE_H
#define SERVER_CORE_H

#include <QTcpServer>
#include <QVector>
#include <QThread>
#include <QJsonObject>
#include "serverworker.h"

class ServerCore : public QTcpServer
{
    Q_OBJECT
    Q_DISABLE_COPY(ServerCore)
public:
    explicit ServerCore(QObject* parent = nullptr);
    ~ServerCore() override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    const int idealThreadCount;
    QVector<QThread*> threads;
    QVector<int> threadLoadFactor;
    QVector<ServerWorker*> clients;
private slots:
    void unicast(const QJsonObject& packet, ServerWorker* receiver);
    void broadcast(const QString& group, const QJsonObject& packet, const ServerWorker* exclude);
    void packetReceived(ServerWorker* sender, const QJsonObject& packet);
    void userDisconnected(ServerWorker* sender, int threadIdx);
    static void userError(ServerWorker* sender);
public slots:
    void stopServer();

private:
    void loginUser(ServerWorker* sender, const QJsonObject& packet);
    bool isUserLoggedIn(const QString& username);
    void registerUser(ServerWorker* sender, const QJsonObject& packet);
    void connectGroup(ServerWorker* sender, const QJsonObject& packet);
    void createGroup(ServerWorker* sender, const QJsonObject& packet);
    void packetFromLoggedOut(ServerWorker* sender, const QJsonObject& packet);
    void packetFromLoggedIn(ServerWorker* sender, const QJsonObject& packet);
    void packetFromConnectedToGroup(ServerWorker* sender, const QJsonObject& packet);
    QJsonArray getUsernames(ServerWorker* exclude) const;
    static QJsonArray getMessages(const QString& groupName);
    static void sendPacket(ServerWorker* destination, const QJsonObject& packet);
    static bool isEqualPacketType(const QJsonValue& jsonType, const char* strType);

signals:
    void stopAllClientsSig();
};

#endif // SERVER_CORE_H

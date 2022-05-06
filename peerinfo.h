#ifndef PEERINFO_H
#define PEERINFO_H

#include "header.h"
#include <QObject>

class PeerInfo : public QObject
{
    Q_OBJECT
public:
    enum PeerType{
        CLIENT = 0,
        SERVER = 1
    };
    enum ONLINESTATUS {
        ONLINE,
        OFFLINE,
        STATIC,
        UNKNOWN
    };

    explicit PeerInfo(
            QObject *parent = 0,
            quint64 id = 0,
            PeerType type = CLIENT,
            QString name = tr("unknown"),
            QHostAddress ip = QHostAddress("127.0.0.1"),
            int port = 0);
    ~PeerInfo();
    // setter
    void setOnline();
    void setOffline();
    void setId(quint64);
    void setName(QString);
    void setIp(QString);
    void setPort(int);

    // getter
    quint64 getId();
    PeerType getType();
    QString getName();
    QString getIp();
    int getPort();
    ONLINESTATUS getStatus();


signals:
    void updated();


public slots:
    void set(quint64 id, QString name, QString ip, int port);
    void updateTimestamp();

private:
    quint64 id;
    enum PeerType type;
    QString name;
    QHostAddress ip;
    int port;
    enum ONLINESTATUS onlineStatus;
    quint64 lastUpdateTimestamp;

};


#endif // PEERINFO_H

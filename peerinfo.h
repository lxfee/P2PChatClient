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
        SERVER = 1,
        UNDEFINED
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
    void setUnknow();
    void setId(quint64);
    void setName(QString);
    void setIp(QString);
    void setPort(int);
    void setType(PeerType);

    // getter
    quint64 getId();
    PeerType getType();
    QString getTypeName();
    QString getName();
    QString getIp();
    quint16 getPort();
    ONLINESTATUS getStatus();
    QString getStatusName();
    quint64 getTimestamp();

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
    quint16 port;
    enum ONLINESTATUS onlineStatus;
    quint64 lastUpdateTimestamp;

};


#endif // PEERINFO_H

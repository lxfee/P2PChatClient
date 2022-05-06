#include "header.h"
#include "peerinfo.h"


PeerInfo::PeerInfo(
        QObject *parent,
        quint64 id,
        PeerType type,
        QString name,
        QHostAddress ip,
        int port) :
    QObject(parent), id(id), type(type), name(name), ip(ip), port(port)
{
    onlineStatus = UNKNOWN;
    connect(this, SIGNAL(updated()), this, SLOT(updateTimestamp()));
    updateTimestamp();
}

PeerInfo::~PeerInfo()
{
}

// getter
void PeerInfo::setOnline() {
    if(onlineStatus == STATIC || onlineStatus == ONLINE) return ;
    onlineStatus = ONLINE;
    emit updated();
}

void PeerInfo::setOffline() {
    if(onlineStatus == STATIC || onlineStatus == OFFLINE) return ;
    onlineStatus = OFFLINE;
    emit updated();
}

void PeerInfo::set(quint64 id, QString name, QString ip, int port) {
    bool changed = false;
    if(this->id != id) {
        this->id = id;
        changed = true;
    }
    if(this->name != name) {
        changed = true;
        this->name = name;
    }
    if(this->ip.toString() != ip) {
        changed = true;
        this->ip = QHostAddress(ip);
    }
    if(this->port != port) {
        changed = true;
        this->port = port;
    }
    if(changed) emit updated();
}

void PeerInfo::setId(quint64 id)
{
    if(this->id == id) return ;
    this->id = id;
    emit updated();
}

void PeerInfo::setName(QString name)
{
    if(this->name == name) return ;
    this->name = name;
    emit updated();
}

void PeerInfo::setIp(QString ip) {
    if(this->ip.toString() == ip) return ;
    this->ip = QHostAddress(ip);
    emit updated();
}

void PeerInfo::setPort(int port) {
    if(this->port == port) return ;
    this->port = port;
    emit updated();
}

// getter
quint64 PeerInfo::getId() {
    return id;
}

PeerInfo::PeerType PeerInfo::getType() {
    return type;
}

QString PeerInfo::getName() {
    return name;
}

QString PeerInfo::getIp()
{
    return ip.toString();
}

int PeerInfo::getPort()
{
    return port;
}

PeerInfo::ONLINESTATUS PeerInfo::getStatus() {
    return onlineStatus;
}


// slot functions

void PeerInfo::updateTimestamp() {
    lastUpdateTimestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

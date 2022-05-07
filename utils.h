#ifndef UTILS_H
#define UTILS_H
#include "header.h"


QList<QString> getHostMacAddressList();
QList<QString> getHostIPList();
QString getBcastIp(QString ip);
quint64 encodeHostMacAddress(QString);
QString decodeHostMacAddress(quint64);
quint64 encodeIPAndPort(QHostAddress, quint16);
QString idToString(quint64);
quint64 stringToid(QString);



#endif // UTILS_H

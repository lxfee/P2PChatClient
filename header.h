#ifndef HEADER_H
#define HEADER_H

#include <QUdpSocket>
#include <QTimer>
#include <QList>
#include <QDateTime>
#include <QtNetwork>
#include <algorithm>
#include <iostream>
#include <QTreeWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMap>
#include <QDateTime>
#include <QModelIndex>
#include <QTranslator>

enum MEESAGETYPE {
    HELLO,
    REPLYHELLO,
    MESSAGE,
    REPLY,
};

struct MessageHeader {
    quint8 messageTye;
    quint8 flag = 0;
    quint16 size;
    unsigned int: 32;
    quint64 from;
    quint64 to;
    quint64 timestamp;
};

struct Message {
    Message(MessageHeader header, QByteArray data);
    MessageHeader header;
    QByteArray message;
    bool recv = false;
    bool operator<(const Message& rhs);
    bool operator==(const Message& rhs);
};

#endif // HEADER_H

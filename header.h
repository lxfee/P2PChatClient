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
    BYE
};

#define PEERTYPE 1
#define ISTRANS 2

struct MessageHeader {
    quint8 messageTye;  // 消息类型
    quint8 flag = 0;    // 标志位
    quint16 size;       // 正文长度
    unsigned int: 32;   // 空闲不使用，用于扩展
    quint64 from;       // 发送方ID
    quint64 to;         // 接收方ID
    quint64 timestamp;  // 消息发送时的时间戳
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

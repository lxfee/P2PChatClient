#include "mainwindow.h"
#include <QApplication>
#include "header.h"
#include "utils.h"



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

Message::Message(MessageHeader header, QByteArray data) : header(header), message(data) {}

bool Message::operator<(const Message &rhs)
{
    return header.timestamp < rhs.header.timestamp;
}

bool Message::operator==(const Message &rhs)
{
    return header.timestamp == rhs.header.timestamp;
}

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include "header.h"
#include "utils.h"
#include "peerinfo.h"

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = 0);
    ~ChatWindow();
    void appendChatWindow(Message& message);
    void reloadChatWindow(QList<Message>& messageList);
    void setRmotePeer(PeerInfo* peer);
    PeerInfo* getRmotePeer();
signals:
    void sendmsg(PeerInfo*, QByteArray);
private slots:
    void on_sendButton_clicked();

private:
    Ui::ChatWindow *ui;
    PeerInfo *remotePeer = nullptr;
};

#endif // CHATWINDOW_H

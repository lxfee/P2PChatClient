#include "chatwindow.h"
#include "ui_chatwindow.h"

ChatWindow::ChatWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::appendChatWindow(Message& message)
{

    if(remotePeer == nullptr) return ;
    quint64 remoteid = remotePeer->getId();
    QString time = QDateTime::fromMSecsSinceEpoch(message.header.timestamp).toString("yyyy-MM-dd hh:mm:ss:zzz");
    QString title;

    if(message.header.to == remoteid) {
        // 发送出去的信息
        title = time + " " + idToString(message.header.from) + tr(" 我");
        title = "<b>" + title + "<\b>";
        if(message.recv) {
            title.append(tr("<font color=\"green\">(已送达)</font>"));
        } else {
            title.append(tr("<font color=\"red\">(已发送)</font>"));
        }

    } else if(message.header.from == remoteid) {
        // 接收到的信息
        title = time + " " + idToString(remoteid) + " " + remotePeer->getName();
        title = "<b>" + title + "<\b>";
    } else return ;

    ui->msgBrowser->append(title);
    ui->msgBrowser->append(message.message);
    ui->msgBrowser->append("\n");
}

void ChatWindow::reloadChatWindow(QList<Message>& messageList)
{
    quint64 remoteid = remotePeer->getId();
    if(!messageList.empty()) {
        if(messageList.last().header.from != remoteid && messageList.last().header.to != remoteid) return ;
    }
    ui->msgBrowser->clear();
    for(int i = 0; i < messageList.size(); i++) {
        appendChatWindow(messageList[i]);
    }
}

void ChatWindow::setRmotePeer(PeerInfo *peer)
{
    remotePeer = peer;
}

PeerInfo *ChatWindow::getRmotePeer()
{
    return remotePeer;
}

void ChatWindow::on_sendButton_clicked()
{
    QString text = ui->msgLineEdit->text();
    ui->msgLineEdit->clear();
    QByteArray data;
    data.append(text);
    emit sendmsg(remotePeer, data);
}

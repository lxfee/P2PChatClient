#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "peerinfo.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)

{
    // setui一定要在第一个
    ui->setupUi(this);

    // 放在setui后面，要设置好this后再来
    settingDiglog = new SettingDialog(this);
    addDialog = new AddDialog(this);
    chatWindow = new ChatWindow();
    local = new PeerInfo(this);
    qDebug() << local->getTypeName();
    broadcastTimer = startTimer(broadcastPeriod);
    updatePeerListTimer = startTimer(updatePeerListPeriod);

    // 设置广播开关
    broadcastswitch = true;
    ui->broButton->setText("关闭广播");

    udpSocket = new QUdpSocket(this);

    connect(chatWindow, SIGNAL(sendmsg(PeerInfo*, QByteArray)), this, SLOT(sendMessage(PeerInfo*, QByteArray)));
    connect(addDialog, SIGNAL(addPeer(quint64,QHostAddress,quint16,QString,PeerInfo::PeerType, PeerInfo::ONLINESTATUS)), this, SLOT(addPeer(quint64,QHostAddress,quint16,QString,PeerInfo::PeerType, PeerInfo::ONLINESTATUS)));
    connect(local, SIGNAL(updated()), this, SLOT(updateLocalInfo()));
    connect(settingDiglog, SIGNAL(update(quint64, QString, QString, int)), local, SLOT(set(quint64, QString, QString, int)));
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));
    connect(ui->peerList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemPressedSlot(QModelIndex)));
    connect(this, SIGNAL(peerupdated(PeerInfo *)), this, SLOT(updateChatWindowInfo(PeerInfo*)));

    initLocalPeerInfo();

}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == broadcastTimer) {
        broadcast();
    }
    if(event->timerId() == updatePeerListTimer) {
        updatePeerList();
    }
}



void MainWindow::initLocalPeerInfo()
{
    QList<QString> ips = getHostIPList();
    QList<QString> macs = getHostMacAddressList();
    QString ip = "127.0.0.1";
    QString mac = "00:00:00:00:00:00";
    int port = 11451;
    QString name = QHostInfo::localHostName();
    if(!ips.empty()) ip = ips.first();
    if(!macs.empty()) mac = macs.first();
    local->set(encodeHostMacAddress(mac), name, ip, port);

}

void MainWindow::processSendMessage(MessageHeader* header, char *data, QHostAddress addr, quint16 port)
{
    addr = QHostAddress(addr.toIPv4Address());
    if(header->messageTye != REPLY)
        header->timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QByteArray datagram((char*)header, sizeof(MessageHeader));
    if(data) {
        datagram.append(data, header->size);
    }

    udpSocket->writeDatagram(datagram.data(), datagram.size(), addr, port);
}



void MainWindow::processDatagram(MessageHeader *header, char *data, QHostAddress addr, quint16 port)
{
    if(header->to && header->to != local->getId()) return ;
    if(header->from && header->from == local->getId()) return ;
     //qDebug() << "type: " << (MEESAGETYPE)header->messageTye <<
     //               "recv from " << header->from << ":" << tr("%1:%2").arg(addr.toString()).arg(port);

    QByteArray recvdata;
    switch(header->messageTye) {
        case HELLO:
            recvdata.append(data, header->size);
            addPeer(header->from, addr, port, recvdata, (PeerInfo::PeerType)(header->flag & PEERTYPE));
            // 回应
            header->messageTye = (qint8)REPLYHELLO;
            header->to = header->from;
            header->from = local->getId();
            recvdata.clear();
            recvdata.append(local->getName());
            header->size = recvdata.size();
            processSendMessage(header, recvdata.data(), addr, port);
        break;

        case REPLYHELLO:
            recvdata.append(data, header->size);
            addPeer(header->from, addr, port, recvdata, (PeerInfo::PeerType)(header->flag & PEERTYPE));
        break;
        case MESSAGE:
            recvdata.append(data, header->size);
            recvMessage(*header, recvdata);

            header->messageTye = (qint8)REPLY;
            header->to = header->from;
            header->from = local->getId();
            recvdata.clear();
            header->size = recvdata.size();
            processSendMessage(header, recvdata.data(), addr, port);
        case REPLY:
            qDebug() << "reply";
            updateReplyMessage(header);
        break;
    }
}

void MainWindow::broadcast()
{
    if(broadcastswitch) {
        qDebug() << "local bro: " << broadcastAddress;
        sendHello(QHostAddress(broadcastAddress), local->getPort());
    }
    // 向其它客户端发送信息保持在线状态
    PeerInfo *peer;
    foreach (peer, peerList) {
        if(peer->getStatus() != PeerInfo::OFFLINE) {
            sendHello(QHostAddress(peer->getIp()), peer->getPort());
        }
    }
}

void MainWindow::updatePeerList()
{

    bool modified = false;
    PeerInfo *peer = nullptr;
    foreach (peer, peerList) {
        if(peer->getStatus() == PeerInfo::ONLINE) {
            int diff = QDateTime::currentDateTime().toMSecsSinceEpoch() - peer->getTimestamp();
            if(diff > 3 * offLineLimit) {
                peer->setOffline();
                modified = true;
            } else {
                sendHello(QHostAddress(peer->getIp()), peer->getPort());
            }
        }
    }
    if(modified) reloadPeerList(peer, peer);
}

void MainWindow::addPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type, PeerInfo::ONLINESTATUS status)
{
    PeerInfo *peer = nullptr;
    PeerInfo *newpeer = nullptr;
    if(status == PeerInfo::UNKNOWN) {
        foreach (peer, peerList) {
            if(encodeIPAndPort(QHostAddress(peer->getIp()), peer->getPort()) == encodeIPAndPort(ip, port)) {
                peer->setUnknow();
                reloadPeerList(peer, peer);
                return ;
            }
        }
    }


    peer = getPeerById(id);
    if(!peer) {
        peer = getPeerById(encodeIPAndPort(ip, port));
    }

    if(peer) {
        PeerInfo oldp(0, peer->getId());
        peer->setId(id);
        peer->setType(type);
        peer->setIp(ip.toString());
        peer->setPort(port);
        peer->setName(name);
        peer->updateTimestamp();
        if(status == PeerInfo::ONLINE)
            peer->setOnline();
        else
            peer->setUnknow();
        reloadPeerList(&oldp, peer);
    } else {
        newpeer = new PeerInfo(this, id, type, name, ip, port);
        if(status == PeerInfo::ONLINE)
            newpeer->setOnline();
        else
            newpeer->setUnknow();
        reloadPeerList(nullptr, newpeer);
    }
}

void MainWindow::setItem(quint64 id, PeerInfo* peer) {
    if(peer == nullptr) return ;
    int size = ui->peerList->topLevelItemCount();
    QTreeWidgetItem *target = nullptr;
    for(int i = 0; i < size; i++) {
        if(stringToid(ui->peerList->topLevelItem(i)->text(0)) == id) {
            target = ui->peerList->topLevelItem(i);
            break;
        }
    }

    if(target) {
        int unread = unReadBox[peer->getId()];
        QString unreadMessage;
        if(unread) {
            unreadMessage = tr("(你有%1条未读信息)").arg(unread);
        }

        target->setText(0, tr("%1").arg(peer->getId()));
        target->setText(1, peer->getTypeName());
        target->setText(2, peer->getName());
        target->setText(3, tr("%1:%2").arg(peer->getIp()).arg(peer->getPort()));
        target->setText(4, peer->getStatusName() + unreadMessage);
        emit peerupdated(peer);
    } else {
        qDebug() << "in setItem, something wrong!";
    }

}

void MainWindow::delItem(PeerInfo *peer)
{
    if(chatWindow->isHidden() || chatWindow->getRmotePeer() != peer) {
        int size = ui->peerList->topLevelItemCount();
        int i = 0;
        for(;i < size; i++) {
            if(stringToid(ui->peerList->topLevelItem(i)->text(0)) == peer->getId()) {
                break;
            }
        }
        if(i != size) {
            delete ui->peerList->takeTopLevelItem(i);
            qDebug() << "delete item" << i;
            if(!peerList.removeOne(peer)) {
                qDebug() << "in delItem, something wrong!";
            }
        }
    } else {
        QMessageBox::warning(this, tr("警告"),
                    tr("打开聊天窗口时无法删除对方"));
    }
}
PeerInfo *MainWindow::getPeerById(quint64 id)
{
    PeerInfo* peer = nullptr;
    bool found = false;
    foreach (peer, peerList) {
        if(peer->getId() == id) {
            found = true;
            break;
        }
    }
    if(found) return peer;
    else {
        return nullptr;
    }
}

PeerInfo *MainWindow::getPeerById(QString id)
{
    return getPeerById(stringToid(id));
}

void MainWindow::addItem(PeerInfo* peer) {
    if(getPeerById(peer->getId())) return ;
    int size = ui->peerList->topLevelItemCount();
    QTreeWidgetItem *child = nullptr;
    for(int i = 0; i < size; i++) {
        child = ui->peerList->topLevelItem(i);
        if(stringToid(child->text(0)) == peer->getId()) qDebug() << "in addItem something wrong!";
    }

    peerList.append(peer);
    int unread = unReadBox[peer->getId()];
    QString unreadMessage;
    if(unread) {
        unreadMessage = tr("(你有%1条未读信息)").arg(unread);
    }

    QStringList texts;
    texts << idToString(peer->getId())
          << peer->getTypeName()
          << peer->getName()
          << QString("%1:%2").arg(peer->getIp()).arg(peer->getPort())
          << peer->getStatusName() + unreadMessage;
    QTreeWidgetItem* info = new QTreeWidgetItem(texts);
    ui->peerList->insertTopLevelItem(0, info);
}

void MainWindow::reloadPeerList(PeerInfo* oldp, PeerInfo* newp)
{
    if(!oldp && !newp) return ;

    if(oldp && newp) {
        setItem(oldp->getId(), newp);
        return ;
    }
    if(newp) {
        addItem(newp);
        return ;
    }
    if(oldp) delItem(oldp);
}

void MainWindow::openChatWindow(PeerInfo *peer)
{
    if(peer == nullptr) return ;
    if(peer->getType() != PeerInfo::CLIENT) {
        QMessageBox::warning(this, tr("错误"),
                    tr("无法向非客户端发起聊天"));
        return ;
    }
    // 使用手动
    chatWindow->setWindowTitle(tr("%1 %2(%3)").arg(peer->getId()).arg(peer->getName()).arg(peer->getStatusName()));
    QList<Message>& list = messageBox[peer->getId()];
    if(peer->getStatus() == PeerInfo::OFFLINE) {
        peer->setUnknow();
        reloadPeerList(peer, peer);
    }
    unreadclear(peer->getId());
    chatWindow->setRmotePeer(peer);
    chatWindow->reloadChatWindow(list);
    chatWindow->show();
}

void MainWindow::addMessage(Message& message)
{
    quint64 remoteid;
    if(local->getId() == message.header.to) {
        // 接收到的信息
        remoteid = message.header.from;
    } else if(local->getId() == message.header.from) {
        // 发送出去的信息
        remoteid = message.header.to;
    } else return ;

    QList<Message>& messageList = messageBox[remoteid];

    int i = messageList.size() - 1;
    bool sorted = false;
    for(; i >= 0; i--) {
        if(message == messageList[i]) return ;
        if(messageList[i] < message) {
            break;
        }
        sorted = true;
    }
    messageList.insert(i + 1, message);
    if(sorted) {
        chatWindow->reloadChatWindow(messageList);
    } else {
        chatWindow->appendChatWindow(message);
    }
}

void MainWindow::unreadins(quint64 id)
{
    if(chatWindow->isHidden() || stringToid(chatWindow->windowTitle()) != id) {
        unReadBox[id]++;
        setItem(id, getPeerById(id));
    }
}

void MainWindow::unreadclear(quint64 id)
{
    unReadBox[id] = 0;
    setItem(id, getPeerById(id));
}

void MainWindow::updateReplyMessage(MessageHeader *header)
{
    quint64 remoteid = header->from;
    if(messageBox.count(remoteid)) {
        QList<Message>& messageList = messageBox[remoteid];
        for(int i = 0; i < messageList.size(); i++) {
            if(messageList[i].header.timestamp == header->timestamp) {
                messageList[i].recv = true;
                break;
            }
        }
        chatWindow->reloadChatWindow(messageList);
    }
}

void MainWindow::sendHello(QHostAddress addr, quint16 port)
{
    MessageHeader header;

    header.messageTye = HELLO;
    header.from = local->getId();
    header.to = 0;
    QByteArray data;
    data.append(local->getName());
    header.size = (quint16)data.size();
    processSendMessage(&header, data.data(), addr, port);
}

void MainWindow::sendMessage(PeerInfo* peer, QByteArray data)
{
    MessageHeader header;

    header.messageTye = MESSAGE;
    header.from = local->getId();
    header.to = peer->getId();
    header.size = (quint16)data.size();
    processSendMessage(&header, data.data(), QHostAddress(peer->getIp()), peer->getPort());
    Message message(header, data);
    addMessage(message);
}

void MainWindow::recvMessage(MessageHeader& header, QByteArray& data)
{
    Message message(header, data);
    addMessage(message);
    unreadins(header.from);
}

void MainWindow::updateLocalInfo()
{
    ui->usernameInfoLabel->setText(local->getName());
    ui->ipInfoLabel->setText(tr("%1:%2").arg(local->getIp()).arg(local->getPort()));
    ui->statusBar->showMessage(tr("ID: %1").arg(local->getId()));
    broadcastAddress = getBcastIp(local->getIp());
    udpSocket->close();
    udpSocket->open(QIODevice::ReadWrite);
    if(!udpSocket->bind(QHostAddress(local->getIp()), local->getPort(), QUdpSocket::ShareAddress)) {
        qDebug() << "can not open socket";
    }
}

void MainWindow::on_settingBtn_clicked()
{
    settingDiglog->loadLocalInfo(local);
    settingDiglog->show();
}

void MainWindow::processPendingDatagram()
{
    while(udpSocket->hasPendingDatagrams()) {

//        datagram.resize(udpSocket->pendingDatagramSize());
        // readDatagram 在5.9版本有bug，谨慎使用。无法正确获得发送方的ip地址
        // 见 https://blog.csdn.net/qq_31073871/article/details/102973655
//        udpSocket->readDatagram(datagram.data(), datagram.size(), &addr, &port);

        QNetworkDatagram netData = udpSocket->receiveDatagram(udpSocket->pendingDatagramSize());
        QHostAddress addr = netData.senderAddress();
        quint16 port = netData.senderPort();


        QByteArray datagram = netData.data();
        if(datagram.size() < (int)sizeof(MessageHeader)) {
            return ;
        }

        MessageHeader* header = (MessageHeader*)datagram.data();
        if(datagram.size() < (int)sizeof(MessageHeader) + header->size) {
            return ;
        }
        if(header->from == local->getId()) return ;
        char* message = datagram.data() + sizeof(MessageHeader);
        processDatagram(header, message, addr, port);
    }
}

void MainWindow::on_mannulBtn_clicked()
{
    addDialog->exec();
}

void MainWindow::on_broButton_clicked()
{
    broadcastswitch ^= 1;
    if(broadcastswitch) ui->broButton->setText("关闭广播");
    else ui->broButton->setText("打开广播");
}


void MainWindow::on_delButton_clicked()
{
    if(ui->peerList->currentItem() == nullptr) return ;
    PeerInfo *peer = getPeerById(ui->peerList->currentItem()->text(0));
    reloadPeerList(peer, nullptr);
}

void MainWindow::itemPressedSlot(QModelIndex index)
{
    if(ui->peerList->currentItem()) {
        openChatWindow(getPeerById(ui->peerList->currentItem()->text(0)));
    }
}

void MainWindow::updateChatWindowInfo(PeerInfo *peer)
{
    if(peer == nullptr) return ;
    if(stringToid(chatWindow->windowTitle()) != peer->getId()) return ;
    chatWindow->setWindowTitle(tr("%1 %2(%3)").arg(peer->getId()).arg(peer->getName()).arg(peer->getStatusName()));
}

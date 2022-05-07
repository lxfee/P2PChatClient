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
    local = new PeerInfo(this);
    qDebug() << local->getTypeName();
    broadcastTimer = startTimer(broadcastPeriod);
    updatePeerListTimer = startTimer(updatePeerListPeriod);

    // 设置广播开关
    broadcastswitch = true;
    ui->broButton->setText("关闭广播");

    udpSocket = new QUdpSocket(this);


    connect(addDialog, SIGNAL(addPeer(quint64,QHostAddress,quint16,QString,PeerInfo::PeerType)), this, SLOT(addUPeer(quint64,QHostAddress,quint16,QString,PeerInfo::PeerType)));
    connect(local, SIGNAL(updated()), this, SLOT(updateLocalInfo()));
    connect(settingDiglog, SIGNAL(update(quint64, QString, QString, int)), local, SLOT(set(quint64, QString, QString, int)));
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));

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

void MainWindow::processSendMessage(MainWindow::MessageHeader* header, char *data, QHostAddress addr, quint16 port)
{
    addr = QHostAddress(addr.toIPv4Address());
    qDebug() << "type " << header->messageTye << " ";
    qDebug() << "send to:" << tr("%1:%2").arg(addr.toString()).arg(port) << "from :" << tr("%1:%2").arg(udpSocket->localAddress().toString()).arg(udpSocket->localPort());
    header->timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QByteArray datagram((char*)header, sizeof(MainWindow::MessageHeader));
    if(data) {
        datagram.append(data, header->size);
    }

    udpSocket->writeDatagram(datagram.data(), datagram.size(), addr, port);
}



void MainWindow::processDatagram(MainWindow::MessageHeader *header, char *data, QHostAddress addr, quint16 port)
{
    if(header->to && header->to != local->getId()) return ;
    if(header->from && header->from == local->getId()) return ;

//    qDebug() << "type: " << (MEESAGETYPE)header->messageTye <<
//                "recv from " << header->from << ":" << tr("%1:%2").arg(addr.toString()).arg(port);

    QByteArray username;
    switch(header->messageTye) {
        case HELLO:
            username.append(data, header->size);
            addPeer(header->from, addr, port, username, (PeerInfo::PeerType)(header->flag & PEERTYPE));
            // 回应
            header->messageTye = (qint8)REPLYHELLO;
            header->to = header->from;
            header->from = local->getId();
            username.clear();
            username.append(local->getName());
            processSendMessage(header, username.data(), addr, port);
        break;

        case REPLYHELLO:
            username.append(data, header->size);
            addPeer(header->from, addr, port, username, (PeerInfo::PeerType)(header->flag & PEERTYPE));
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

void MainWindow::addPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type)
{
    PeerInfo *peer = getPeerById(id);
    PeerInfo *newpeer = nullptr;

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
        peer->setOnline();
        reloadPeerList(&oldp, peer);
    } else {
        newpeer = new PeerInfo(this, id, type, name, ip, port);
        newpeer->setOnline();
        reloadPeerList(nullptr, newpeer);
    }
}

void MainWindow::addUPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type)
{
    PeerInfo *peer = getPeerById(id);
    PeerInfo *newpeer = nullptr;
    if(peer) {
        PeerInfo oldp(0, peer->getId());
        peer->setId(id);
        peer->setIp(ip.toString());
        peer->setPort(port);
        peer->setName(name);
        peer->updateTimestamp();
        peer->setUnknow();
        reloadPeerList(&oldp, peer);
    } else {
        newpeer = new PeerInfo(this, id, type, name, ip, port);
        newpeer->setUnknow();
        reloadPeerList(nullptr, newpeer);
    }
}

void MainWindow::setItem(quint64 id, PeerInfo* peer) {
    int p = 0;
    QTreeWidgetItem* target =  nullptr;
    while((target = ui->peerList->itemAt(0, p)) != nullptr) {
        if(target->text(0) == tr("%1").arg(id)) break;
        p++;
    }
    if(target) {
        target->setText(0, tr("%1").arg(peer->getId()));
        target->setText(1, peer->getTypeName());
        target->setText(2, peer->getName());
        target->setText(3, tr("%1:%2").arg(peer->getIp()).arg(peer->getPort()));
        target->setText(4, peer->getStatusName());
    }

}

void MainWindow::delItem(PeerInfo *peer)
{
    QTreeWidgetItem* target =  nullptr;
    int p = 0;
    while((target = ui->peerList->itemAt(0, p)) != nullptr) {
        if(target->text(0) == tr("%1").arg(peer->getId())) break;
        p++;
    }
    delete ui->peerList->takeTopLevelItem(p);
    peerList.removeOne(peer);
}

PeerInfo *MainWindow::getPeerById(quint64 id)
{
    return getPeerById(tr("%1").arg(id));
}

PeerInfo *MainWindow::getPeerById(QString id)
{
    PeerInfo* peer = nullptr;
    bool found = false;
    foreach (peer, peerList) {
        if(tr("%1").arg(peer->getId()) == id) {
            found = true;
            break;
        }
    }
    if(found) return peer;
    else {
        return nullptr;
    }
}

void MainWindow::addItem(PeerInfo* peer) {
    peerList.append(peer);
    QStringList texts;
    texts << tr("%1").arg(peer->getId())
          << peer->getTypeName()
          << peer->getName()
          << tr("%1:%2").arg(peer->getIp()).arg(peer->getPort())
          << peer->getStatusName();
    QTreeWidgetItem* info = new QTreeWidgetItem(texts);
    ui->peerList->insertTopLevelItem(0, info);
}

void MainWindow::reloadPeerList(PeerInfo* oldp, PeerInfo* newp)
{
    if(!oldp && !newp) {
        return ;
    }
    if(oldp && newp) {
        setItem(oldp->getId(), newp);
        return ;
    }
    if(newp) {
        addItem(newp);
        return ;
    }
    if(oldp) {
        delItem(oldp);
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
    settingDiglog->exec();
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

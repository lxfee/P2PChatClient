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

    broadcaster = new QUdpSocket(this);
    udpSocket = new QUdpSocket(this);


    connect(addDialog, SIGNAL(addPeer(quint64,QHostAddress,quint16,QString,PeerInfo::PeerType)), this, SLOT(addUPeer(quint64,QHostAddress,quint16,QString,PeerInfo::PeerType)));
    connect(local, SIGNAL(updated()), this, SLOT(updateLocalInfo()));
    connect(settingDiglog, SIGNAL(update(quint64, QString, QString, int)), local, SLOT(set(quint64, QString, QString, int)));
    connect(broadcaster, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));


    initLocalPeerInfo();


    broadcaster->bind(QHostAddress::AnyIPv4, RECVPORT, QUdpSocket::ShareAddress);
    udpSocket->bind(QHostAddress(local->getIp()), local->getPort(), QUdpSocket::ShareAddress);

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
    qDebug() << "send to:" << tr("%1:%2").arg(addr.toString()).arg(port) << "from :" << tr("%1:%2").arg(udpSocket->localAddress().toString()).arg(udpSocket->localPort());
    header->timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QByteArray datagram((char*)header, sizeof(MainWindow::MessageHeader));
    if(data) {
        datagram.append(data, header->size);
    }

    udpSocket->writeDatagram(datagram.data(), datagram.size(), addr, port);
}

void MainWindow::parseDatagram(QUdpSocket *sock)
{ 
    if(sock->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress addr;
        quint16 port;
        datagram.resize(sock->pendingDatagramSize());
        sock->readDatagram(datagram.data(), datagram.size(), &addr, &port);
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

void MainWindow::processDatagram(MainWindow::MessageHeader *header, char *data, QHostAddress addr, quint16 port)
{
    if(header->to && header->to != local->getId()) return ;
    QByteArray username;
    switch(header->messageTye) {
        case HELLO:
            username.append(data, header->size);
            addPeer(header->from, addr, port, username, (PeerInfo::PeerType)(header->flag & PEERTYPE));

            qDebug() << "recv from " << header->from << "[" << username << "]" << ":" << tr("%1:%2").arg(addr.toString()).arg(port);
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
    sendHello(QHostAddress(broadcastAddress), (quint16)RECVPORT);
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
    PeerInfo *peer;
    foreach (peer, peerList) {
        if(peer->getStatus() == PeerInfo::ONLINE) {
            int diff = QDateTime::currentDateTime().toMSecsSinceEpoch() - peer->getTimestamp();
            if(diff > offLineLimit) {
                peer->setOffline();
                modified = true;
            }
        }
    }
    if(modified) reloadPeerList();
}

void MainWindow::addPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type)
{
    PeerInfo *peer = nullptr;
    bool found = false;
    foreach (peer, peerList) {
        if(peer->getId() == id) {
            found = true;
            break;
        }
    }
    if(found) {
        peer->setIp(ip.toString());
        peer->setPort(port);
        peer->setName(name);
        peer->updateTimestamp();
        peer->setOnline();
    } else {
        peer = new PeerInfo(this, id, type, name, ip, port);
        peer->setOnline();
        peerList.append(peer);
    }

    reloadPeerList();
}

void MainWindow::addUPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type)
{
    PeerInfo *peer = nullptr;
    bool found = false;
    foreach (peer, peerList) {
        if(peer->getId() == id) {
            found = true;
            break;
        }
    }
    if(found) {
        peer->setIp(ip.toString());
        peer->setPort(port);
        peer->setName(name);
        peer->updateTimestamp();
        peer->setUnknow();
    } else {
        peer = new PeerInfo(this, id, type, name, ip, port);
        peer->setUnknow();
        peerList.append(peer);
    }

    reloadPeerList();
}

void MainWindow::reloadPeerList()
{
    PeerInfo *peer;
    ui->peerList->clear();
    foreach (peer, peerList) {
        QStringList texts;
        texts << tr("%1").arg(peer->getId())
              << peer->getTypeName()
              << peer->getName()
              << tr("%1:%2").arg(peer->getIp()).arg(peer->getPort())
              << peer->getStatusName();
        QTreeWidgetItem* info = new QTreeWidgetItem(texts);
        ui->peerList->insertTopLevelItem(0, info);
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
    udpSocket->bind(QHostAddress(local->getIp()), local->getPort(), QUdpSocket::ShareAddress);
    qDebug() << "udpSocket: " << udpSocket->state();
    qDebug() << "broadcaster: " << broadcaster->state();
}

void MainWindow::on_settingBtn_clicked()
{
    settingDiglog->loadLocalInfo(local);
    settingDiglog->exec();
}

void MainWindow::processPendingDatagram()
{
    while(udpSocket->hasPendingDatagrams()) {
        qDebug() << "udpSocket recv:";
        parseDatagram(udpSocket);
    }
    while(broadcaster->hasPendingDatagrams()) {
        qDebug() << "broadcaster recv:";
        parseDatagram(broadcaster);
    }
}

void MainWindow::on_mannulBtn_clicked()
{
    addDialog->exec();
}

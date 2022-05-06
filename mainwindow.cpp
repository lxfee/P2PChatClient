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
    local = new PeerInfo(this);

    broadcastTimer = startTimer(broadcastPeriod);
    broadcaster = new QUdpSocket(this);
    udpSocket = new QUdpSocket(this);


    connect(local, SIGNAL(updated()), this, SLOT(updateLocalInfo()));
    connect(settingDiglog, SIGNAL(update(quint64, QString, QString, int)), local, SLOT(set(quint64, QString, QString, int)));
    connect(broadcaster, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));

    initLocalPeerInfo();



    broadcaster->bind(RECVPORT, QUdpSocket::ShareAddress);
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
    qDebug() << "send datagram";
    header->timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QByteArray datagram((char*)header, sizeof(MainWindow::MessageHeader));
    if(data) {
        datagram.append(data, header->size);
    }

    switch(header->messageTye) {
        case HELLO:

            udpSocket->writeDatagram(datagram.data(), datagram.size(), addr, port);
        break;
    }
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
    switch(header->messageTye) {
        case HELLO:
            QByteArray username(data, header->size);
            addPeer(header->from, addr, port, username, (PeerInfo::PeerType)(header->flag & PEERTYPE));
            // 回应
            header->messageTye = (qint8)REPLYHELLO;
            header->to = header->from;
            header->from = local->getId();
            username.clear();
            username.append(local->getName());
            processSendMessage(header, username.data(), addr, port);
        break;

    }
}

void MainWindow::broadcast()
{
    MessageHeader header;

    header.messageTye = HELLO;
    header.from = local->getId();
    header.to = 0;
    QByteArray data;
    data.append(local->getName());
    header.size = (quint16)data.size();
    processSendMessage(&header, data.data(), QHostAddress(broadcastAddress), (quint16)RECVPORT);
}

void MainWindow::addPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type)
{
    PeerInfo *peer = nullptr;
    foreach (peer, peerList) {
        if(peer->getId() == id) {
            break;
        }
    }
    if(peer) {
        peer->setIp(ip.toString());
        peer->setPort(port);
        peer->setName(name);
        peer->updateTimestamp();
    } else {
        peer = new PeerInfo(this, id, type, name, ip, port);
        peerList.append(peer);
    }

    reloadPeerList();
}

void MainWindow::reloadPeerList()
{
    PeerInfo *peer;
    ui->peerList->clear();
    foreach (peer, peerList) {

        ui->peerList->addItem(peer->getName());
    }
}

void MainWindow::updateLocalInfo()
{
    ui->usernameInfoLabel->setText(local->getName());
    ui->ipInfoLabel->setText(tr("%1:%2").arg(local->getIp()).arg(local->getPort()));
    ui->statusBar->showMessage(tr("ID: %1").arg(local->getId()));
    broadcastAddress = getBcastIp(local->getIp());
    udpSocket->close();
    udpSocket->bind(QHostAddress(local->getIp()), local->getPort(), QUdpSocket::ShareAddress);
}

void MainWindow::on_settingBtn_clicked()
{
    settingDiglog->loadLocalInfo(local);
    settingDiglog->exec();
}

void MainWindow::processPendingDatagram()
{
    while(udpSocket->hasPendingDatagrams()) {
        parseDatagram(udpSocket);
    }
    while(broadcaster->hasPendingDatagrams()) {
        parseDatagram(broadcaster);
    }
}

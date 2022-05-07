#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "header.h"
#include "peerinfo.h"
#include "settingdialog.h"
#include "adddialog.h"

#define PEERTYPE 1

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    enum MEESAGETYPE {
        HELLO,
        REPLYHELLO,
        MESSAGE,
        TEXT,
        IMAGE,
        FILE,
        GAME,
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

protected:
    void timerEvent(QTimerEvent *);



private slots:

    void on_settingBtn_clicked();
    // 处理接收到的udp包
    void processPendingDatagram();
    // 更新显示本地信息
    void updateLocalInfo();
    // 添加Peer
    void addPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type);
    void addUPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type);

    void on_mannulBtn_clicked();

    void on_broButton_clicked();

    void on_delButton_clicked();

private:
    Ui::MainWindow *ui;
    SettingDialog *settingDiglog;
    AddDialog *addDialog;

    // 初始化本地网络信息
    void initLocalPeerInfo();

    // 处理要发送的数据
    void processSendMessage(MessageHeader* header, char* data, QHostAddress addr, quint16 port);
    void processDatagram(MainWindow::MessageHeader *header, char *data, QHostAddress addr, quint16 port);

    // 定时器
    int broadcastTimer;                     // 广播自己状态定时器
    const int broadcastPeriod = 1000;       // 广播自己状态周期
    void broadcast();                       // 广播+发送udp在线状态
    bool broadcastswitch;

    int updatePeerListTimer;                // 检查更新列表定时器
    const int updatePeerListPeriod = 5000;  // 检查更新列表周期
    void updatePeerList();                  // 更新列表
    const int offLineLimit = 10000;         // 离线时间10s


    // 列表相关
    QList<PeerInfo*> peerList;
    void addItem(PeerInfo* peer);
    void setItem(quint64 id, PeerInfo* peer);
    void delItem(PeerInfo* peer);
    PeerInfo* getPeerById(quint64 id);
    PeerInfo* getPeerById(QString id);

    void reloadPeerList(PeerInfo* oldp, PeerInfo* newp);

    // 本地客户端信息
    PeerInfo* local;
    QString broadcastAddress;

    // udp socket
    QUdpSocket* udpSocket;

    // 不同操作
    void sendHello(QHostAddress addr, quint16 port);



};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "header.h"
#include "peerinfo.h"
#include "settingdialog.h"
#include "adddialog.h"
#include "chatwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    // 时钟事件
    void timerEvent(QTimerEvent *);
    // 关闭事件
    void closeEvent(QCloseEvent *);
signals:
    // 发生列表更新操作
    void peerupdated(PeerInfo * newpeer);

private slots:
    // 设置按钮按下
    void on_settingBtn_clicked();
    // 添加按钮按下
    void on_mannulBtn_clicked();
    // 广播按钮按下
    void on_broButton_clicked();
    // 删除按钮按下
    void on_delButton_clicked();
    // 处理接收到的udp包
    void processPendingDatagram();
    // 更新显示本地信息
    void updateLocalInfo();
    // 添加列表项目
    void addPeer(quint64 id, QHostAddress ip, quint16 port, QString name, PeerInfo::PeerType type, PeerInfo::ONLINESTATUS status = PeerInfo::ONLINE);
    // 双击选中列表
    void itemPressedSlot(QModelIndex index);
    // 实时更新聊天窗口中用户信息
    void updateChatWindowInfo(PeerInfo *peer);
    // 发送消息
    void sendMessage(PeerInfo* peer, QByteArray data);

private:
    // 窗口对象
    Ui::MainWindow *ui;
    SettingDialog *settingDiglog;
    AddDialog *addDialog;
    ChatWindow *chatWindow;

    // 初始化本地网络信息
    void initLocalPeerInfo();
    // 处理要发送的数据
    void processSendMessage(MessageHeader* header, char* data, QHostAddress addr, quint16 port);
    // 处理接收的数据
    void processDatagram(MessageHeader *header, char *data, QHostAddress addr, quint16 port);

    // 定时器
    int broadcastTimer;                     // 广播自己状态定时器
    const int broadcastPeriod = 1000;       // 广播自己状态周期
    void broadcast();                       // 广播+发送udp在线状态
    bool broadcastswitch;                   // 广播开关

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
    PeerInfo* local;            // 本地地址:端口
    QString broadcastAddress;   // 广播地址

    // udp 套接字
    QUdpSocket* udpSocket;


    // 聊天相关
    void openChatWindow(PeerInfo *peer);
    void addMessage(Message& message);
    // 存储消息的容器
    QMap<quint64, QList<Message>> messageBox;
    QMap<quint64, int> unReadBox;
    void unreadins(quint64 id);
    void unreadclear(quint64 id);
    // 更新响应
    void updateReplyMessage(MessageHeader*);

    // 具体操作
    void sendHello(QHostAddress addr, quint16 port);
    void sendBye(QHostAddress addr, quint16 port);
    void recvMessage(MessageHeader& header, QByteArray& data);

    bool mode = true;

};

#endif // MAINWINDOW_H

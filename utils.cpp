#include "utils.h"

QList<QString> getHostMacAddressList()
{
    QList<QString> macList;
    // 获取所有网络接口列表
    QList<QNetworkInterface> nets = QNetworkInterface::allInterfaces();
    int nCnt = nets.count();
    for(int i = 0; i < nCnt; i ++) {
        // 如果此网络接口被激活并且正在运行并且不是回环地址，则就是我们需要找的Mac地址
        if(nets[i].flags().testFlag(QNetworkInterface::IsUp) &&
                nets[i].flags().testFlag(QNetworkInterface::IsRunning) &&
                !nets[i].flags().testFlag(QNetworkInterface::IsLoopBack)) {
            macList.append(nets[i].hardwareAddress());
        }
    }
    return macList;
}


QList<QString> getHostIPList()
{
    QList<QString> ips;
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    for (int i = 0; i < interfaceList.count(); i++) {
        QNetworkInterface interf = interfaceList.at(i);
        interf.humanReadableName(); //接口名称（网卡）
        QList<QNetworkAddressEntry> entryList = interf.addressEntries(); // 读取一个IP地址列表
        for(int j = 0; j < entryList.count(); j++) {
            QNetworkAddressEntry entry = entryList.at(j);
            if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                QString ip = entry.ip().toString();                   //IP地址
                ips.append(ip);
            }
        }
    }
    return ips;
}


quint64 encodeHostMacAddress(QString mac) {
    quint64 id = 0;
    QList<QString> bytes = mac.split(':');
    QString byte;
    foreach (byte, bytes) {
        byte.toUpper();
        if(byte.size() != 2) return id;
        id = (id << 8) + byte.toInt(nullptr, 16);
    }
    return id;
}

QString decodeHostMacAddress(quint64 macid)
{
    const char* hex = "0123456789ABCDEF";
    QString mac;
    for(int i = 0; i < 6; i++) {
        int byte = macid & (255);
        if(i) mac.append(':');
        macid = (macid >> 8);
        mac.append(hex[byte&15]);
        byte = byte >> 4;
        mac.append(hex[byte&15]);
    }
    std::reverse(mac.begin(), mac.end());
    return mac;
}

QString getBcastIp(QString ip)
{
    QString bc = "255.255.255.255";
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    for (int i = 0; i < interfaceList.count(); i++) {
        QNetworkInterface interf = interfaceList.at(i);
        interf.humanReadableName(); //接口名称（网卡）
        QList<QNetworkAddressEntry> entryList = interf.addressEntries(); // 读取一个IP地址列表
        for(int j = 0; j < entryList.count(); j++) {
            QNetworkAddressEntry entry = entryList.at(j);
            if(entry.ip().toString() == ip) {
                bc = entry.broadcast().toString();
                if(bc.size() == 0) bc = "255.255.255.255";
                break;
            }
        }
    }
    return bc;
}

quint64 encodeIPAndPort(QHostAddress ip, quint16 port)
{
    quint64 id = ip.toIPv4Address();
    return (id << 16) + port;
}


QString idToString(quint64 id) {
    return QString("%1").arg(id);
}

quint64 stringToid(QString id) {
    quint64 nid = 0;
    id = id.simplified();
    std::string sid = id.toStdString();
    for(int i = 0; i < (int)sid.size(); i++) {
        if(!isdigit(sid[i])) break;
        nid = 10 * nid + sid[i] - '0';
    }
    return nid;
}

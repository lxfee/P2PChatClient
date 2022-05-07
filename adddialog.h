#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include <QDialog>
#include "header.h"
#include "peerinfo.h"

namespace Ui {
class AddDialog;
}

class AddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDialog(QWidget *parent = 0);
    ~AddDialog();
signals:
    void addPeer(quint64, QHostAddress, quint16, QString, PeerInfo::PeerType);

private slots:
    void on_addBtn_clicked();


    
private:
    Ui::AddDialog *ui;
};

#endif // ADDDIALOG_H

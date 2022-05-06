#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include "header.h"
#include "utils.h"
#include "peerinfo.h"

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(QWidget *parent = 0);
    ~SettingDialog();
    void loadLocalInfo(PeerInfo*);

signals:
    void update(quint64, QString, QString, int);


private slots:
    void on_settingBtn_clicked();

private:
    Ui::SettingDialog *ui;
};

#endif // SETTINGDIALOG_H

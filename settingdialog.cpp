#include "settingdialog.h"
#include "ui_settingdialog.h"

SettingDialog::SettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);

    QList<QString> ips = getHostIPList();
    ui->ipBoBox->addItems(ips);

    QList<QString> macs = getHostMacAddressList();
    ui->macBoBox->addItems(macs);

}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::loadLocalInfo(PeerInfo* local)
{
    QString mac = decodeHostMacAddress(local->getId());
    QString ip = local->getIp();
    auto ips = getHostIPList();

    ui->usrLineEdit->setText(local->getName());
    ui->portSpinBox->setValue(local->getPort());

    int pos = ui->macBoBox->findText(mac);
    if(pos == -1) {
        ui->macBoBox->clear();
        QList<QString> macs = getHostMacAddressList();
        ui->macBoBox->addItems(macs);
    }
    ui->macBoBox->setCurrentText(mac);

    pos = ui->ipBoBox->findText(ip);

    bool found = ips.contains(ip);
    pos = ui->ipBoBox->findText(ip);
    if(!found || pos == -1) {
        ui->ipBoBox->clear();
        ui->ipBoBox->addItems(ips);
    }
    ui->ipBoBox->setCurrentText(ip);
}

void SettingDialog::on_settingBtn_clicked()
{
    emit update(encodeHostMacAddress(ui->macBoBox->currentText()), ui->usrLineEdit->text(), ui->ipBoBox->currentText(), ui->portSpinBox->value());
    close();
}

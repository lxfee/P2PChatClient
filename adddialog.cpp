#include "adddialog.h"
#include "ui_adddialog.h"

AddDialog::AddDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDialog)
{
    ui->setupUi(this);
}

AddDialog::~AddDialog()
{
    delete ui;
}

void AddDialog::on_addBtn_clicked()
{
    QHostAddress ip = QHostAddress(ui->ipLineEdit->text());
    quint16 port = ui->portSpinBox->value();
    quint64 id = encodeIPAndPort(ip, port);
    emit addPeer(id, ip, port, tr("未知"), PeerInfo::UNDEFINED, PeerInfo::UNKNOWN);
    close();
}

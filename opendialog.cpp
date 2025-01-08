/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#include "opendialog.h"
#include "ui_opendialog.h"
#include "pvnlab.h"
#include <QFileInfo>
#include <QFileDialog>

OpenDialog::OpenDialog(QWidget *parent, QString ip, int port, QStringList serialPorts, QString map, quint16 interval, quint16 offset, quint16 maxLength,
		QString commDevice, quint16 slave, QSerialPort::BaudRate baudrate, QSerialPort::Parity parity, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits,
		quint32 timeout, quint16 retries, PvnlabQModbus::ModbusType modbusType) : QDialog(parent), m_ui(new Ui::OpenDialog){
	//PVNLAB_DEBUG;
	m_ui->setupUi(this);

	m_ui->leIp->setText(ip);
	m_ui->sbPort->setValue(port);
	m_ui->leMap->setText(map);
	m_ui->sbInterval->setValue(interval);
	m_ui->sbOffset->setValue(offset);
	m_ui->sbMaxLength->setValue(maxLength);
	m_ui->cbCommDevice->addItems(serialPorts);
	int index = serialPorts.indexOf(commDevice);
	if(index > 0){
		m_ui->cbCommDevice->setCurrentIndex(index);
	}
	m_ui->sbSlave->setValue(slave);
	m_ui->cbBaudrate->setCurrentIndex(baudrate);
	m_ui->cbParity->setCurrentIndex(parity);
	m_ui->cbDatabits->setCurrentIndex(databits);
	m_ui->cbStopbits->setCurrentIndex(stopbits);
	m_ui->sbTimeout->setValue(int(timeout));
	m_ui->sbRetries->setValue(retries);
	m_ui->gbTcp->setChecked(modbusType == PvnlabQModbus::Tcp);
	m_ui->gbRtu->setChecked(modbusType == PvnlabQModbus::Rtu);

	m_map = map;

	connect(m_ui->gbTcp, SIGNAL(toggled(bool)), this, SLOT(tcpSelected(bool)));
	connect(m_ui->gbRtu, SIGNAL(toggled(bool)), this, SLOT(rtuSelected(bool)));
	connect(m_ui->tbMap, SIGNAL(clicked()), this, SLOT(chooseMap()));
}

OpenDialog::~OpenDialog(){
	//PVNLAB_DEBUG;
	delete m_ui;
}

QString OpenDialog::ip()					{	return m_ui->leIp->text();}
QString OpenDialog::map()					{	return m_ui->leMap->text();}
quint16 OpenDialog::interval()					{	return quint16(m_ui->sbInterval->value());}
quint16 OpenDialog::offset()					{	return quint16(m_ui->sbOffset->value());}
quint16 OpenDialog::maxLength()					{	return quint16(m_ui->sbMaxLength->value());}
QString OpenDialog::commDevice()				{	return m_ui->cbCommDevice->currentText();}
quint16 OpenDialog::slave()					{	return quint16(m_ui->sbSlave->value());}
QSerialPort::BaudRate OpenDialog::baudrate()			{	return QSerialPort::BaudRate(m_ui->cbBaudrate->currentIndex());}
QSerialPort::Parity OpenDialog::parity()			{	return QSerialPort::Parity(m_ui->cbParity->currentIndex());}
QSerialPort::DataBits OpenDialog::databits()			{	return QSerialPort::DataBits(m_ui->cbDatabits->currentIndex());}
QSerialPort::StopBits OpenDialog::stopbits()			{	return QSerialPort::StopBits(m_ui->cbStopbits->currentIndex());}
quint32 OpenDialog::timeout()					{	return quint32(m_ui->sbTimeout->value());}
quint16 OpenDialog::retries()					{	return quint16(m_ui->sbRetries->value());}
quint16 OpenDialog::port()					{	return quint16(m_ui->sbPort->value());}

PvnlabQModbus::ModbusType OpenDialog::modbusType(){
	if(m_ui->gbRtu->isChecked()){
		return PvnlabQModbus::Rtu;
	}
	else if(m_ui->gbTcp->isChecked()){
		return PvnlabQModbus::Tcp;
	}
	else{
		return PvnlabQModbus::ModbusTypeUnknown;
	}
}

void OpenDialog::tcpSelected(bool checked){
	//PVNLAB_DEBUG;
	m_ui->gbRtu->setChecked(not checked);
}

void OpenDialog::rtuSelected(bool checked){
	//PVNLAB_DEBUG;
	m_ui->gbTcp->setChecked(not checked);
}

void OpenDialog::chooseMap(){
	QFileInfo finfo(m_map);
	QString path = QFileDialog::getOpenFileName(this, tr("Mappe di DruidLAB"), finfo.absolutePath(), tr("Map file (*.json *.she)"));
	if (path != ""){
		m_ui->leMap->setText(path);
	}
}

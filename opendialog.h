/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <QDialog>
#include <QSerialPort>
#include "pvnlab_qmodbus.h"

QT_BEGIN_NAMESPACE

namespace Ui {
	class OpenDialog;
}

QT_END_NAMESPACE

class OpenDialog : public QDialog
{
	Q_OBJECT

public:
	explicit OpenDialog(QWidget *parent, QString ip, int port, QStringList serialPorts, QString map = "",
				quint16 interval = 1000, quint16 offset = 0, quint16 maxLength = 20, QString commDevice = "", quint16 slave = 1,
				QSerialPort::BaudRate baudrate = QSerialPort::Baud9600, QSerialPort::Parity parity = QSerialPort::NoParity,
				QSerialPort::DataBits databit = QSerialPort::Data8, QSerialPort::StopBits stopbits = QSerialPort::OneStop,
				quint32 timeout = 50, quint16 retries = 1, PvnlabQModbus::ModbusType modbusType = PvnlabQModbus::Tcp);
	~OpenDialog();

	QString ip();
	quint16 port();
	QString map();
	quint16 interval();
	quint16 offset();
	quint16 maxLength();
	QString commDevice();
	quint16 slave();
	QSerialPort::BaudRate baudrate();
	QSerialPort::Parity parity();
	QSerialPort::DataBits databits();
	QSerialPort::StopBits stopbits();
	quint16 retries();
	quint32 timeout();
	PvnlabQModbus::ModbusType modbusType();

private slots:
	void tcpSelected(bool checked);
	void rtuSelected(bool checked);
	void chooseMap();


private:
	Ui::OpenDialog *m_ui;
	QString m_map;


};

#endif // OPENDIALOG_H

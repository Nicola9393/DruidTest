/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QSerialPort>
#include <QModbusDevice>
#include "pvnlab_qmodbus.h"

QT_BEGIN_NAMESPACE

namespace Ui {
	class MainWindow;
}


class PvnlabQModbusThreadedClient;
class PvnlabQModbusDevice;
class QTreeWidgetItem;
class DeviceWidget;

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	//void importMaps();
	void update();
	void about();
	void help();
	void modbusConnect();
	void modbusDisconnect();
	void showTraceTab();
	void showRegistersTab();
	void showPersonalizedTab();
	void onModbusConnected();
	void onModbusDisconnected();
	void onClientErrorOccured(QModbusDevice::Error error);
	void onClientStateChanged(QModbusDevice::State state);
	void onClientReadError(QString errstr);
	void setDarkMode(bool dark);

private:
	void closeEvent(QCloseEvent *e);
	void readSettings();
	void saveSettings();
	void updateWindowTitle();
	QString logHeader();

	Ui::MainWindow* m_ui;
	QString m_appdir;
	QString m_cfg;
	QString m_mapdir;

	PvnlabQModbusThreadedClient* m_modbus;
	PvnlabQModbusDevice* m_device;
	DeviceWidget* m_devWidget;
	QString m_ip;
	quint16 m_port;
	QString m_map;
	quint16 m_interval;
	quint16 m_offset;
	quint16 m_maxLength;
	QString m_commDevice;
	quint16 m_slave;
	QSerialPort::BaudRate m_baudrate;
	QSerialPort::Parity m_parity;
	QSerialPort::DataBits m_databits;
	QSerialPort::StopBits m_stopbits;
	quint32 m_timeout;
	quint16 m_retries;
	PvnlabQModbus::ModbusType m_modbusType;
	bool m_darkmode;
	bool m_fusionStyle;
};

#endif // MAINWINDOW_H


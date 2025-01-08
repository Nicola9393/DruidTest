/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QSettings>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include <QInputDialog>
#include <QProgressDialog>
#include <QTimer>
#include <QToolTip>
#include <QFontDatabase>
#include <QThread>
#include <QMetaEnum>
#include <QDateTime>
#include <QStyleFactory>

#include "pvnlab.h"
#include "pvnlab_qmodbus_client.h"
#include "pvnlab_qmodbus_threaded_client.h"
#include "pvnlab_qmodbus_device.h"
#include "pvnlab_qmodbus_item.h"
#include "pvnlab_about_dialog.h"
#include "devicewidget.h"
#include "opendialog.h"
#include "pvnlab_app_lock.h"

#include <QModbusTcpClient>
#include <QUrl>
#include "pvnlab.h"

#define DARKER 110


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),	m_ui(new Ui::MainWindow){
	//PVNLAB_DEBUG;
	m_ui->setupUi(this);

	m_appdir = QDir::homePath() + "/.piovan/druid_lab";
	m_mapdir = QDir::homePath() + "/.piovan/druid_lab/json_maps";
	m_cfg = m_appdir+ "/druid_lab.cfg";

	readSettings();

	m_modbus = nullptr;
	m_device = nullptr;
	m_devWidget = nullptr;

	connect(m_ui->actionQuit, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	connect(m_ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
	connect(m_ui->actionHelp, SIGNAL(triggered()), this, SLOT(help()));
	connect(m_ui->actionConnect, SIGNAL(triggered()), this, SLOT(modbusConnect()));
	connect(m_ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(modbusDisconnect()));
	connect(m_ui->actionShowTrace, SIGNAL(triggered()), this, SLOT(showTraceTab()));
	connect(m_ui->actionShowRegisters, SIGNAL(triggered()), this, SLOT(showRegistersTab()));
	connect(m_ui->actionShowPersonalized, SIGNAL(triggered()), this, SLOT(showPersonalizedTab()));
	connect(m_ui->actionDarkMode, SIGNAL(triggered(bool)), this, SLOT(setDarkMode(bool)));

	m_ui->actionDarkMode->setChecked(m_darkmode);

	updateWindowTitle();

	QTimer::singleShot(100, this, SLOT(modbusConnect()));

}

MainWindow::~MainWindow(){
	//PVNLAB_DEBUG;
	if(m_device != nullptr){
		delete m_device;
	}
	if(m_modbus != nullptr){
		delete m_modbus;
	}
	if(m_devWidget != nullptr){
		delete m_devWidget;
	}
	delete m_ui;
}

void MainWindow::closeEvent(QCloseEvent *e){
	//PVNLAB_DEBUG;
	saveSettings();
	modbusDisconnect();
	e->accept();
}

void MainWindow::saveSettings(){
	//PVNLAB_DEBUG;
	QSettings settings(m_cfg , QSettings::IniFormat);

	#if defined(Q_OS_WIN)
	settings.beginGroup("Style");
	settings.setValue("fusionStyle", m_fusionStyle);
	settings.setValue("darkmode", m_darkmode);
	settings.endGroup();
	#elif defined (Q_OS_UNIX)
	settings.beginGroup("Style");
	settings.setValue("darkmode", m_darkmode);
	settings.endGroup();
	#endif

	settings.beginGroup(QCoreApplication::applicationName());
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.setValue("ip", m_ip);
	settings.setValue("port", m_port);
	settings.setValue("map", m_map);
	settings.setValue("interval", m_interval);
	settings.setValue("offset", m_offset);
	settings.setValue("maxLength", m_maxLength);
	settings.setValue("commDevice", m_commDevice);
	settings.setValue("slave", m_slave);
	settings.setValue("baudrate", m_baudrate);
	settings.setValue("parity", m_parity);
	settings.setValue("databits", m_databits);
	settings.setValue("stopbits", m_stopbits);
	settings.setValue("timeout", m_timeout);
	settings.setValue("modbusType", m_modbusType);
	settings.setValue("retries", m_retries);

	settings.endGroup();
}

void MainWindow::readSettings(){
	//PVNLAB_DEBUG;
	QSettings settings(m_cfg , QSettings::IniFormat);

	#if defined(Q_OS_WIN)
	settings.beginGroup("Style");
	m_fusionStyle = settings.value("fusionStyle", false).toBool();
	m_darkmode = settings.value("darkmode", false).toBool();

	if(m_fusionStyle and not m_darkmode){
		qApp->setStyle(QStyleFactory::create("Fusion"));
	}
	if(m_darkmode){
		qApp->setStyle(QStyleFactory::create("Fusion"));
		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, QColor(53,53,53));
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, QColor(25,25,25));
		darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
		darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, QColor(53,53,53));
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::BrightText, Qt::red);
		darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
		darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
		darkPalette.setColor(QPalette::HighlightedText, Qt::black);
		qApp->setPalette(darkPalette);
		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	settings.endGroup();
	#elif defined (Q_OS_UNIX)
	settings.beginGroup("Style");
	m_fusionStyle = false;
	m_darkmode = settings.value("darkmode", false).toBool();
	settings.endGroup();
	#endif

	settings.beginGroup(QCoreApplication::applicationName());
	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());
	m_ip = settings.value("ip", "192.168.13.4").toString();
	m_port = quint16(settings.value("port", "502").toUInt());
	m_map = settings.value("map", "").toString();
	m_interval = quint16(settings.value("interval", 5).toUInt());
	m_offset = quint16(settings.value("offset", 0).toUInt());
	m_maxLength = quint16(settings.value("maxLength", 20).toUInt());
	m_commDevice = settings.value("commDevice", "COM1").toString();
	m_slave = quint16(settings.value("slave", 1).toUInt());
	m_baudrate = QSerialPort::BaudRate(settings.value("baudrate", QSerialPort::Baud57600).toInt());
	m_parity = QSerialPort::Parity(settings.value("parity", QSerialPort::NoParity).toInt());
	m_databits = QSerialPort::DataBits(settings.value("databits", QSerialPort::Data8).toInt());
	m_stopbits = QSerialPort::StopBits(settings.value("stopbits", QSerialPort::OneStop).toInt());
	m_timeout = quint32(settings.value("timeout", 1).toUInt());
	m_modbusType = static_cast<PvnlabQModbus::ModbusType>(settings.value("modbusType", 1).toInt());
	m_retries = quint16(settings.value("retries", 1).toInt());
	settings.endGroup();
}

void MainWindow::about(){
	//PVNLAB_DEBUG;
	QString title = QCoreApplication::applicationName() + " - " + tr("versione") + " " + QCoreApplication::applicationVersion() + " - ID: " + PvnlabAppLock::machineID();
	QString info = tr("Copyright (C) 2013 Piovan S.p.a\nAutore:") + "Enricomaria Pavan";
	PvnlabAboutDialog adia(this, tr("Informazioni"), title, info, qApp->applicationDirPath() + tr("/LICENZA.druid_lab"));
	adia.exec();
}

void MainWindow::help(){
	//PVNLAB_DEBUG;
	QString title = QCoreApplication::applicationName() + " - " + tr("Guida");
	QString info = tr("Scorciatoie da tastiera");

	QStringList lines;
	lines << tr("F6\t- Visualizza registri");
	lines << tr("F7\t- Visualizza I/O");
	lines << tr("F8\t- Visualizza log");

	PvnlabAboutDialog adia(this, tr("Aiuto"), title, info, lines);
	adia.exec();
}

void MainWindow::modbusConnect(){
	//PVNLAB_DEBUG;


	if(m_modbus != nullptr){
		modbusDisconnect();
	}

	OpenDialog dialog(this, m_ip, m_port, PvnlabQModbus::availableSerialPorts(), m_map, m_interval, m_offset, m_maxLength,
			m_commDevice, m_slave, m_baudrate, m_parity, m_databits, m_stopbits, m_timeout, m_retries, m_modbusType);
	if(dialog.exec() == QDialog::Accepted){

		QProgressDialog progress(tr("..."), tr(""), 0, 3, this);
		progress.setWindowModality(Qt::WindowModal);
		progress.setCancelButton(nullptr);
		progress.setAutoClose(false);
		progress.show();
		progress.setValue(progress.value() + 1);
		progress.setLabelText(tr("Inizializzazione..."));
		qApp->processEvents();

		m_ip = dialog.ip();
		m_port = dialog.port();
		m_map = dialog.map();
		m_interval = dialog.interval();
		m_offset = dialog.offset();
		m_maxLength = dialog.maxLength();
		m_commDevice = dialog.commDevice();
		m_slave = dialog.slave();
		m_baudrate = dialog.baudrate();
		m_parity = dialog.parity();
		m_databits = dialog.databits();
		m_stopbits = dialog.stopbits();
		m_timeout = dialog.timeout();
		m_retries = dialog.retries();
		m_modbusType = dialog.modbusType();
		saveSettings();

		if(not (m_modbusType == PvnlabQModbus::ModbusTypeUnknown)){
			if(m_modbusType == PvnlabQModbus::Rtu){
				m_modbus = new PvnlabQModbusThreadedClient(nullptr, m_commDevice, m_slave, m_timeout, m_retries, m_baudrate, m_parity, m_databits, m_stopbits, m_interval, m_maxLength);
			}
			else if(m_modbusType == PvnlabQModbus::Tcp){
				m_modbus = new PvnlabQModbusThreadedClient(nullptr, m_ip, m_port, m_slave, m_timeout, m_retries, m_interval, m_maxLength);
			}
			connect(m_modbus, SIGNAL(clientStateChanged(QModbusDevice::State)), this, SLOT(onClientStateChanged(QModbusDevice::State)));
			connect(m_modbus, SIGNAL(clientErrorOccurred(QModbusDevice::Error)), this, SLOT(onClientErrorOccured(QModbusDevice::Error)));
			connect(m_modbus, SIGNAL(clientReadError(QString)), this, SLOT(onClientReadError(QString)));
			connect(m_modbus, SIGNAL(modbusConnected()), this, SLOT(onModbusConnected()));
			connect(m_modbus, SIGNAL(modbusDisconnected()), this, SLOT(onModbusDisconnected()));

			QFileInfo finfo(m_map);
			m_device = new PvnlabQModbusDevice(this, m_modbus, finfo.absolutePath(), finfo.fileName(), PvnlabQModbusDevice::Unknown, 0, m_offset);
			bool done = m_device->initializeFromJSON();
			if(done){
				m_devWidget = new DeviceWidget(this, m_device, m_cfg, m_darkmode);
				m_devWidget->setVisible(false);
				this->centralWidget()->layout()->addWidget(m_devWidget);
				connect(m_modbus, SIGNAL(updated()), m_devWidget, SLOT(update()));
				progress.setValue(progress.value() + 1);
				progress.setLabelText(tr("Connessione modbus"));
				qApp->processEvents();
				// connetti ad update
				// connect(m_modbus, SIGNAL(updated()), this, SLOT(update()));
				// connetti al modbus
				m_ui->dockWidgetConsole->setVisible(true);
				m_modbus->modbusConnect();
				progress.setValue(progress.value() + 1);
				progress.setValue(progress.maximum());
				progress.hide();
			}
			else{
				QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Errore in inizializzazione.") + "\n\n" + m_device->initError());
			}
		}
		else{
			QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Tipo di connessione sconosciuto.") + "\n\n" + m_device->initError());
		}

	}
	if((width() <sizeHint().width()) or (height() < sizeHint().height())){
		resize(sizeHint());
		setMinimumSize(sizeHint());
	}
}

void MainWindow::onModbusConnected(){
	//PVNLAB_DEBUG;
	m_devWidget->setVisible(true);
	m_ui->dockWidgetConsole->setVisible(false);
	updateWindowTitle();
}

void MainWindow::onModbusDisconnected(){
	//PVNLAB_DEBUG;
	if(m_devWidget != nullptr){
		m_devWidget->setVisible(false);
		m_ui->centralWidget->layout()->removeWidget(m_devWidget);
		delete m_devWidget;
		m_devWidget = nullptr;
	}
	disconnect(m_modbus, SIGNAL(clientStateChanged(QModbusDevice::State)), this, SLOT(onClientStateChanged(QModbusDevice::State)));
	disconnect(m_modbus, SIGNAL(clientErrorOccurred(QModbusDevice::Error)), this, SLOT(onClientErrorOccured(QModbusDevice::Error)));
	disconnect(m_modbus, SIGNAL(clientReadError(QString)), this, SLOT(onClientReadError(QString)));
	disconnect(m_modbus, SIGNAL(modbusConnected()), this, SLOT(onModbusConnected()));
	disconnect(m_modbus, SIGNAL(modbusDisconnected()), this, SLOT(onModbusDisconnected()));
	delete m_modbus;
	m_modbus = nullptr;
	updateWindowTitle();
}

QString MainWindow::logHeader(){
	//PVNLAB_DEBUG;
	QString line = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "\t";
	if(m_modbus){
		if(m_modbusType == PvnlabQModbus::Tcp){
			line = line + m_modbus->ipAndPort() + " ";
		}
		else if(m_modbusType == PvnlabQModbus::Rtu){
			line = line + m_modbus->commDevice() + " ";
		}
		line = line + tr("slave id: ") + QString::number(m_modbus->slave()) + "\t";
	}
	else{
		line = line + "nullptr client";
	}
	return line;

}

void MainWindow::onClientErrorOccured(QModbusDevice::Error error){
	//PVNLAB_DEBUG;
	QMetaEnum metaEnum = QMetaEnum::fromType<QModbusDevice::Error>();
	QString line = logHeader() + tr("Errore generico:") + "\t"+  metaEnum.valueToKey(error);
	m_ui->textEditConsole->setTextColor(Qt::red);
	m_ui->textEditConsole->insertPlainText(line + "\n");
	m_ui->textEditConsole->ensureCursorVisible();
	m_ui->dockWidgetConsole->setVisible(true);
	statusBar()->showMessage(line, 1000);
}

void MainWindow::onClientStateChanged(QModbusDevice::State state){
	//PVNLAB_DEBUG;
	if(state == QModbusDevice::UnconnectedState){
		m_modbus->deleteLater();
	}
	QMetaEnum metaEnum = QMetaEnum::fromType<QModbusDevice::State>();
	QString line = logHeader() + tr("Stato interfaccia:") + "\t"+ metaEnum.valueToKey(state);
	m_ui->textEditConsole->setTextColor(Qt::lightGray);
	m_ui->textEditConsole->insertPlainText(line + "\n");
	m_ui->textEditConsole->ensureCursorVisible();
	statusBar()->showMessage(line, 1000);
	// disconnetti se cade la connessione
	if(state == QModbusDevice::ClosingState){
		modbusDisconnect();
	}
}

void MainWindow::onClientReadError(QString errstr){
	//PVNLAB_DEBUG;
	QString line = logHeader() + tr("Errore lettura:") + "\t"+ errstr;
	m_ui->textEditConsole->setTextColor(Qt::yellow);
	m_ui->textEditConsole->insertPlainText(line + "\n");
	m_ui->textEditConsole->ensureCursorVisible();
	m_ui->dockWidgetConsole->setVisible(true);
	statusBar()->showMessage(line, 1000);
}

void MainWindow::modbusDisconnect(){
	//PVNLAB_DEBUG;
	QProgressDialog progress(tr("Disconnessione..."), tr(""), 0, 31, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setCancelButton(nullptr);
	progress.setAutoClose(false);
	progress.setRange(0, 5);
	progress.show();
	qApp->processEvents();

	if(m_devWidget != nullptr){
		m_devWidget->setVisible(false);
		this->centralWidget()->layout()->removeWidget(m_devWidget);

		progress.setValue(progress.value() + 1);
		QThread::msleep(10);			// solo per far vedere il progress...
	}
	delete m_devWidget;
	m_devWidget = nullptr;

	if(m_modbus != nullptr){
		if(m_modbus->isModbusConnected()){
			m_ui->dockWidgetConsole->setVisible(true);
			//disconnect(m_modbus, SIGNAL(clientStateChanged(QModbusDevice::State)), this, SLOT(onClientStateChanged(QModbusDevice::State)));
			//disconnect(m_modbus, SIGNAL(clientErrorOccurred(QModbusDevice::Error)), this, SLOT(onClientErrorOccured(QModbusDevice::Error)));
			//disconnect(m_modbus, SIGNAL(clientReadError(QString)), this, SLOT(onClientReadError(QString)));
			//disconnect(m_modbus, SIGNAL(modbusConnected()), this, SLOT(onModbusConnected()));
			m_modbus->modbusDisconnect();
		}
	}
	progress.setValue(progress.value() + 1);
	progress.setValue(progress.maximum());
	progress.hide();
}

void MainWindow::update(){
	//PVNLAB_DEBUG;
}

void MainWindow::showTraceTab(){
	//PVNLAB_DEBUG;
	m_devWidget->setCurrentTab(TAB_TRACE);
}

void MainWindow::showRegistersTab(){
	//PVNLAB_DEBUG;
	m_devWidget->setCurrentTab(TAB_REG);
}

void MainWindow::showPersonalizedTab(){
	//PVNLAB_DEBUG;
	m_devWidget->setCurrentTab(TAB_PERSONAL);
}

void MainWindow::updateWindowTitle(){
	//PVNLAB_DEBUG;
	QString title = QCoreApplication::applicationName();
	if(m_modbus != nullptr){
		if(m_modbus->isModbusConnected()){
			if(m_modbusType == PvnlabQModbus::Rtu){
				title += " - " + m_commDevice;
				title += " (" + QString::number(m_slave) + ")";
				title += " - " + QString::number(m_baudrate);
				title += ", " + QString::number(m_databits);
				title += ", " + QString(m_parity);
				title += ", " + QString::number(m_stopbits);
				title += " - " + m_map;
				title += " - " + QString::number(m_offset);
			}
			else if(m_modbusType == PvnlabQModbus::Tcp){
				title += " - " + m_ip;
				title += " - " + m_map;
				title += " - " + QString::number(m_offset);
			}
		}
	}
	this->setWindowTitle(title);
}

void MainWindow::setDarkMode(bool dark){
	//PVNLAB_DEBUG;
	m_darkmode = dark;
	QMessageBox::information(this, qApp->applicationName(), tr("Riavviare l'applicazione per rendere effettiva la modifica."));
}

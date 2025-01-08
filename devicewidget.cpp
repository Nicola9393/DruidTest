/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#include <QShortcut>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QInputDialog>
#include <QSettings>
#include <QDate>
#include <QDateTime>
#include <QThread>

#include <qwt_plot_curve.h>
#include <qwt_series_store.h>
#include <qwt_symbol.h>
#include <qwt_date.h>
#include <qwt_date_scale_draw.h>
#include <qwt_date_scale_engine.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>

#include "devicewidget.h"
#include "ui_devicewidget.h"
#include "tracewidget.h"
#include "logger.h"
#include "inputdialog.h"
#include "pvnlab.h"
#include "pvnlab_qmodbus_device.h"
#include "pvnlab_qmodbus_item.h"
#include "tableupdater.h"


DeviceWidget::DeviceWidget(QWidget *parent, PvnlabQModbusDevice *device, QString cfg, bool darmkode): QWidget(parent), m_ui(new Ui::DeviceWidget){
	//PVNLAB_DEBUG;
	m_device = device;
	m_cfg = cfg;
	m_darkmode = darmkode;

	m_traceColors[0] = PVNLAB_NEON_CYAN;
	m_traceColors[1] = PVNLAB_NEON_YELLOW;
	m_traceColors[2] = PVNLAB_NEON_RED;
	m_traceColors[3] = PVNLAB_NEON_GREEN;
	m_traceColors[4] = PVNLAB_NEON_ORANGE;
	m_traceColors[5] = PVNLAB_NEON_FUCHSIA;

	m_logging = false;
	m_tableRegisterUpdateThread = nullptr;
	m_tableInputUpdateThread = nullptr;
	m_tableOutputUpdateThread = nullptr;
	m_tableDebugUpdateThread = nullptr;
	m_tablePersonalizedUpdateThread = nullptr;

	m_ui->setupUi(this);

	m_plotDateDraw = new QwtDateScaleDraw();
	m_plotDateDraw->setDateFormat(QwtDate::Minute, "hh:mm:ss\nyyyy-MM-dd");
	m_ui->qwtPlot->setAxisScaleDraw(QwtPlot::xBottom, m_plotDateDraw);
	m_ui->qwtPlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtDateScaleEngine());
	QFont font = m_ui->qwtPlot->axisWidget(QwtPlot::xBottom)->font();
	font.setPointSize(7);
	m_ui->qwtPlot->enableAxis(QwtPlot::xBottom);
	m_ui->qwtPlot->enableAxis(QwtPlot::yLeft);
	m_ui->qwtPlot->enableAxis(QwtPlot::yRight);
	m_ui->qwtPlot->axisWidget(QwtPlot::xBottom)->setFont(font);
	m_ui->qwtPlot->axisWidget(QwtPlot::yLeft)->setFont(font);
	m_ui->qwtPlot->axisWidget(QwtPlot::yRight)->setFont(font);
	//m_plotLegend = new QwtLegend();
	//m_ui->qwtPlot->insertLegend(m_plotLegend, QwtPlot::BottomLegend);
	m_plotZoomer = new QwtPlotZoomer(m_ui->qwtPlot->canvas());
	m_plotZoomer->setRubberBandPen(PVNLAB_WHITE);
	m_plotZoomer->setTrackerPen(PVNLAB_WHITE);

	QString version = "";
	m_ui->tabWidget->setCurrentIndex(0);
	m_ui->checkBoxShowDebug->setChecked(false);
	m_ui->frameDebug->setVisible(false);

	new QShortcut(QKeySequence(Qt::Key_Up), this, SLOT(selectPrevRow()));
	new QShortcut(QKeySequence(Qt::Key_Down), this, SLOT(selectNextRow()));
	new QShortcut(QKeySequence(Qt::Key_Return),  this, SLOT(changeValue()));
	new QShortcut(QKeySequence(Qt::Key_Escape),  m_ui->lineEditFilter, SLOT(clear()));

	readSettings();
	initialize();

	connect(m_ui->pushButtonStart, SIGNAL(clicked()), m_device, SLOT(start()));
	connect(m_ui->pushButtonStop, SIGNAL(clicked()), m_device, SLOT(stop()));
	connect(m_ui->pushButtonSnooze, SIGNAL(clicked()), m_device, SLOT(snooze()));
	connect(m_ui->pushButtonAck, SIGNAL(clicked()), m_device, SLOT(ack()));
        connect(m_ui->pushButtonTestStart, SIGNAL(clicked()), m_device, SLOT(testModeStart()));
        connect(m_ui->pushButtonTestStop, SIGNAL(clicked()), m_device, SLOT(testModeStop()));
	connect(m_ui->tableWidgetRegisters, SIGNAL(itemActivated(QTableWidgetItem*)), this, SLOT(changeValue(QTableWidgetItem*)));
	connect(m_ui->tableWidgetDebug, SIGNAL(itemActivated(QTableWidgetItem*)), this, SLOT(changeValue(QTableWidgetItem*)));
	connect(m_ui->tableWidgetPersonalized, SIGNAL(itemActivated(QTableWidgetItem*)), this, SLOT(changeValue(QTableWidgetItem*)));
	connect(m_ui->lineEditFilter, SIGNAL(textChanged(QString)), this, SLOT(filterRegistersTable(QString)));
	connect(m_ui->pushButtonPersonalizedAdd, SIGNAL(clicked()), this, SLOT(addToPersonalized()));
	connect(m_ui->pushButtonPersonalizedRemove, SIGNAL(clicked()), this, SLOT(removeFromPersonalized()));
	connect(m_ui->pushButtonPersonalizedClear, SIGNAL(clicked()), this, SLOT(clearPersonalized()));
	connect(m_ui->pushButtonPersonalizedAddAll, SIGNAL(clicked()), this, SLOT(addAllToPersonalized()));
	connect(m_ui->pushButtonRec, SIGNAL(clicked()), this, SLOT(recLog()));
	connect(m_ui->pushButtonRecStop, SIGNAL(clicked()), this, SLOT(stopLog()));
	connect(m_ui->pushButtonSaveRegisters, SIGNAL(clicked()), this, SLOT(saveRegisters()));
	connect(m_ui->pushButtonUploadRegisters, SIGNAL(clicked()), this, SLOT(uploadRegisters()));

	m_traceTimer = new QTimer();
	connect(m_ui->comboBoxTraceSampleTime, SIGNAL(currentIndexChanged(int)), this, SLOT(sampleTimeChanged(int)));
	connect(m_traceTimer, SIGNAL(timeout()), this, SLOT(updateTrace()));
	m_traceTimer->start(int(m_ui->comboBoxTraceSampleTime->currentText().toDouble() * 1000.0));

	m_ui->tableWidgetRegisters->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui->tableWidgetInputs->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui->tableWidgetOutputs->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui->tableWidgetDebug->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui->tableWidgetPersonalized->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_ui->tableWidgetRegisters, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showCustomMenuMain(QPoint)));
	connect(m_ui->tableWidgetInputs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showCustomMenuInputs(QPoint)));
	connect(m_ui->tableWidgetOutputs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showCustomMenuOutputs(QPoint)));
	connect(m_ui->tableWidgetDebug, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showCustomMenuDebug(QPoint)));
	connect(m_ui->tableWidgetPersonalized, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showCustomMenuPersonalized(QPoint)));

	connect(m_ui->pushButtonTraceClear, SIGNAL(clicked()), this, SLOT(clearTraces()));
	connect(m_ui->comboBoxTraceSpanTime, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePlotScales()));
	connect(m_ui->pushButtonTraceResetZoom, SIGNAL(clicked(bool)), this, SLOT(resetZoom()));
	connect(m_plotZoomer, SIGNAL(zoomed(QRectF)), this, SLOT(traceZoomed()));

	m_logTimer = new QTimer();

	m_ui->labelRunning->setVisible(m_device->containsTag("RUNNING"));
	m_ui->labelWarning->setVisible(m_device->containsTag("WARNING"));
	m_ui->labelFailure->setVisible(m_device->containsTag("FAILURE"));
	m_ui->labelTestMode->setVisible(m_device->containsTag("TEST_MODE"));
	m_ui->labelTestMode2->setVisible(m_device->containsTag("TEST_MODE"));
	m_ui->pushButtonStart->setVisible(m_device->containsTag("START"));
	m_ui->pushButtonStop->setVisible(m_device->containsTag("STOP"));
	m_ui->pushButtonAck->setVisible(m_device->containsTag("ACK"));
	m_ui->pushButtonSnooze->setVisible(m_device->containsTag("SNOOZE"));
	m_ui->pushButtonTestStart->setVisible(m_device->containsTag("TEST_START"));
	m_ui->pushButtonTestStop->setVisible(m_device->containsTag("TEST_STOP"));
}

DeviceWidget::~DeviceWidget(){
	//PVNLAB_DEBUG;
	saveSettings();
	stopLog();
	m_traceTimer->stop();

	reallyQuitTableThread(m_tableRegisterUpdateThread);
	reallyQuitTableThread(m_tableInputUpdateThread);
	reallyQuitTableThread(m_tableOutputUpdateThread);
	reallyQuitTableThread(m_tableDebugUpdateThread);
	reallyQuitTableThread(m_tablePersonalizedUpdateThread);
	qApp->processEvents();

	delete m_logTimer;
	delete m_traceTimer;	
	delete m_ui;
}


void DeviceWidget::reallyQuitTableThread(QThread *thread){
	//PVNLAB_DEBUG;
	if(thread != nullptr){
		quint16 count = 0;
		while(!thread->isFinished()){
			////PVNLAB_DEBUG<<"Quitting table update thread thread "<<count;
			if(count < 250){
				thread->quit();
				QThread::msleep(50);
			}
			else{
				////PVNLAB_DEBUG<<"Terminating thread!!!! ";
				thread->terminate();
				count = 0;
			}
			count++;
		}
	}
}

void DeviceWidget::setVisible(bool visible){
	//PVNLAB_DEBUG;
	QWidget::setVisible(visible);
	focusToFilter();
	update();
}

void DeviceWidget::saveSettings(){
	//PVNLAB_DEBUG;
	if(m_cfg != ""){
		QSettings settings(m_cfg , QSettings::IniFormat);
		settings.beginGroup("DeviceWidget" + QString::number(m_device->id()));
		settings.setValue("trace_addr0", m_ui->lineEditTraceAddress0->text());
		settings.setValue("trace_addr1", m_ui->lineEditTraceAddress1->text());
		settings.setValue("trace_addr2", m_ui->lineEditTraceAddress2->text());
		settings.setValue("trace_addr3", m_ui->lineEditTraceAddress3->text());
		settings.setValue("trace_addr4", m_ui->lineEditTraceAddress4->text());
		settings.setValue("trace_addr5", m_ui->lineEditTraceAddress5->text());
		settings.setValue("trace_min1", m_ui->lineEditMin1->text());
		settings.setValue("trace_max1", m_ui->lineEditMax1->text());
		settings.setValue("trace_min2", m_ui->lineEditMin2->text());
		settings.setValue("trace_max2", m_ui->lineEditMax2->text());
		settings.setValue("trace_scale0", m_ui->comboBoxTrace0->currentIndex());
		settings.setValue("trace_scale1", m_ui->comboBoxTrace1->currentIndex());
		settings.setValue("trace_scale2", m_ui->comboBoxTrace2->currentIndex());
		settings.setValue("trace_scale3", m_ui->comboBoxTrace3->currentIndex());
		settings.setValue("trace_scale4", m_ui->comboBoxTrace4->currentIndex());
		settings.setValue("trace_scale5", m_ui->comboBoxTrace5->currentIndex());
		settings.setValue("trace_sampleTime", m_ui->comboBoxTraceSampleTime->currentIndex());
		settings.setValue("personalizedAddresses", m_personalizedAddresses);
		settings.setValue("logDir", m_logDir);
		settings.setValue("registersDir", m_registersDir);

		settings.endGroup();
	}
}

void DeviceWidget::readSettings(){
	//PVNLAB_DEBUG;
	QSettings settings(m_cfg , QSettings::IniFormat);

	settings.beginGroup("DeviceWidget" + QString::number(m_device->id()));
	m_ui->lineEditTraceAddress0->setText(settings.value("trace_addr0", "").toString());
	m_ui->lineEditTraceAddress1->setText(settings.value("trace_addr1", "").toString());
	m_ui->lineEditTraceAddress2->setText(settings.value("trace_addr2", "").toString());
	m_ui->lineEditTraceAddress3->setText(settings.value("trace_addr3", "").toString());
	m_ui->lineEditTraceAddress4->setText(settings.value("trace_addr4", "").toString());
	m_ui->lineEditTraceAddress5->setText(settings.value("trace_addr5", "").toString());
	m_ui->lineEditMin1->setText(settings.value("trace_min1", "0").toString());
	m_ui->lineEditMax1->setText(settings.value("trace_max1", "0").toString());
	m_ui->lineEditMin2->setText(settings.value("trace_min2", "0").toString());
	m_ui->lineEditMax2->setText(settings.value("trace_max2", "0").toString());
	m_ui->comboBoxTrace0->setCurrentIndex(settings.value("trace_scale0", 0).toInt());
	m_ui->comboBoxTrace1->setCurrentIndex(settings.value("trace_scale1", 0).toInt());
	m_ui->comboBoxTrace2->setCurrentIndex(settings.value("trace_scale2", 0).toInt());
	m_ui->comboBoxTrace3->setCurrentIndex(settings.value("trace_scale3", 0).toInt());
	m_ui->comboBoxTrace4->setCurrentIndex(settings.value("trace_scale4", 0).toInt());
	m_ui->comboBoxTrace5->setCurrentIndex(settings.value("trace_scale5", 0).toInt());
	m_ui->comboBoxTraceSampleTime->setCurrentIndex(settings.value("trace_sample", 0).toInt());
	m_personalizedAddresses = settings.value("personalizedAddresses").toStringList();
	m_logDir = settings.value("logDir", "C:\\").toString();
	m_registersDir = settings.value("registersDir", "C:\\").toString();
	settings.endGroup();
}

void DeviceWidget::initialize(){
	//PVNLAB_DEBUG;
	initializeTable(m_ui->tableWidgetRegisters, m_device->sortedAddresses());
	initializeTable(m_ui->tableWidgetInputs, m_device->sortedInsAddresses());
	initializeTable(m_ui->tableWidgetOutputs, m_device->sortedOutsAddresses());
	initializeTable(m_ui->tableWidgetDebug, m_device->sortedDebugAddresses());
	initializeTable(m_ui->tableWidgetPersonalized, m_personalizedAddresses);
}

void DeviceWidget::initializeTable(QTableWidget *table, QStringList addresses){
	//PVNLAB_DEBUG;
	while(table->rowCount() > 0){
		table->removeRow(0);
	}
	table->setColumnCount(6);
	QStringList headers;
	headers << tr("Indirizzo");
	headers << tr("Bit");
	headers << tr("Descrizione");
	headers << tr("Valore");
	headers << tr("Unità");
	headers << tr("");
	table->setHorizontalHeaderLabels(headers);

	bool switcher = false;
	int lastreg = -1;

	foreach (QString addr, addresses){
		if(m_device->containsAddress(addr)){
			PvnlabQModbusItem *item = m_device->itemAt(addr);
			QStringList texts;
			texts<<QString::number(item->reg());
			if(item->bit() >= 0){
				texts<<QString::number(item->bit());
			}
			else{
				texts<<"";
			}
			texts<<item->description();
			texts<<item->value();
			if(item->unit() == "#"){
				texts<<"";
			}
			else{
				texts<<item->unit();
			}
			texts<<"";				// inserisco ultimo elemento vuoto
			table->insertRow(table->rowCount());
			int row = table->rowCount() - 1;
			bool hide = false;
			if(lastreg != item->reg()){
				switcher = not switcher;
				lastreg = item->reg();
			}
			else{
				hide = true;
			}
			for(int j = 0; j < 6; j++){
				QTableWidgetItem *twi = new QTableWidgetItem();
				table->setItem(row, j, twi);
				twi->setText(texts[j]);
				if(item->isWritable()){
					if(switcher){
						if(m_darkmode){
							twi->setBackground(PVNLAB_DARK_PURPLE_1);
						}
						else{
							twi->setBackground(PVNLAB_ORANGE_1);
						}
					}
					else{
						if(m_darkmode){
							twi->setBackground(PVNLAB_DARK_PURPLE_2);
						}
						else{
							twi->setBackground(PVNLAB_ORANGE_2);
						}
					}
				}
				else{
					if(switcher){
						if(m_darkmode){
							twi->setBackground(PVNLAB_DARK_BLUE_1);
						}
						else{
							twi->setBackground(PVNLAB_CYAN_1);
						}
					}
					else{
						if(m_darkmode){
							twi->setBackground(PVNLAB_DARK_BLUE_2);
						}
						else{
							twi->setBackground(PVNLAB_CYAN_2);
						}
					}
				}
				if(j == 0){
					if(hide){
						// farfugliata per nascondere l'indirizzo
						QFont font = twi->font();
						font.setPointSize(7);
						twi->setFont(font);
						twi->setTextAlignment(Qt::AlignRight);
						twi->setTextColor(PVNLAB_GRAY_1);
					}
					else{
						QFont font = twi->font();
						font.setBold(true);
						twi->setFont(font);
					}
				}
				if(j==5){
					// nel tooltip sull'ultima colonna metto un po' di infromazioni utili per pvnn
					QString tip;
					QTextStream stream(&tip, QIODevice::WriteOnly);
					stream<<"Device:"<<"\t"<<item->device()<<endl;
					stream<<"Register:"<<"\t"<<item->reg()<<endl;
					stream<<"Bit:"<<"\t"<<item->bit()<<endl;
					stream<<"Tag:"<<"\t"<<item->tag()<<endl;
					stream<<"Type:"<<"\t"<<item->type()<<endl;
					stream<<"Unit:"<<"\t"<<item->unit()<<endl;
					stream<<"Choices:"<<"\t"<<item->choices().join(":")<<endl;
					stream<<"Factor:"<<"\t"<<item->factor()<<endl;
					stream<<"Writable:"<<"\t"<<item->isWritable()<<endl;
					stream<<"Min:"<<"\t"<<item->min()<<endl;
					stream<<"Max:"<<"\t"<<item->max()<<endl;
					stream<<"Relmin:"<<"\t"<<item->isMinRelative()<<endl;
					stream<<"Relmax:"<<"\t"<<item->isMaxRelative()<<endl;
					twi->setToolTip(tip);
				}
			}
		}
	}
	table->setVisible(false);
	table->resizeColumnsToContents();
	table->setVisible(true);
	table->horizontalHeader()->setStretchLastSection(true);
}

void DeviceWidget::update(){
	//PVNLAB_DEBUG;
	if(isVisible()){
		updateRegisterTable();
		updateInputTable();
		updateOutputTable();
		updateDebugTable();
		updatePersonalizedTable();

		updateAlerts();
		updateStatus();
		updateTraceInfo();
	}
}

QString DeviceWidget::addressAt(QTableWidget *table, int row){
	//PVNLAB_DEBUG;
	QString addr = table->item(row, 0)->text();
	bool ok;
	int bit = table->item(row, 1)->text().toInt(&ok);
	if(ok){
		addr = addr + "." + QString("%1").arg(bit, 2, 10, QChar('0'));
	}
	return addr;
}

void DeviceWidget::updateRegisterTable(){
	//PVNLAB_DEBUG;
	if(m_tableRegisterUpdateThread == NULL){
		TableUpdater *tabler = new TableUpdater(m_device, m_ui->tableWidgetRegisters, COLUMN_VAL);
		QThread* thread = new QThread();
		tabler->moveToThread(thread);
		connect(thread, SIGNAL(started()), tabler, SLOT(process()));
		connect(tabler, SIGNAL(finished()), thread, SLOT(quit()));
		connect(tabler, SIGNAL(finished()), tabler, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
		connect(tabler, SIGNAL(finished()), m_ui->tableWidgetRegisters->viewport(), SLOT(update()));	// serve per rinfrescare la visualizzazione dei dati
		connect(thread, SIGNAL(destroyed()), this, SLOT(tableRegisterUpdateDone()));
		m_tableRegisterUpdateThread = thread;
		thread->start();
	}
}

void DeviceWidget::updateInputTable(){
	//PVNLAB_DEBUG;
	if(m_tableInputUpdateThread == NULL){
		TableUpdater *tabler = new TableUpdater(m_device, m_ui->tableWidgetInputs, COLUMN_VAL);
		QThread* thread = new QThread();
		tabler->moveToThread(thread);
		connect(thread, SIGNAL(started()), tabler, SLOT(process()));
		connect(tabler, SIGNAL(finished()), thread, SLOT(quit()));
		connect(tabler, SIGNAL(finished()), tabler, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
		connect(tabler, SIGNAL(finished()), m_ui->tableWidgetInputs->viewport(), SLOT(update()));	// serve per rinfrescare la visualizzazione dei dati
		connect(thread, SIGNAL(destroyed()), this, SLOT(tableInputUpdateDone()));
		m_tableInputUpdateThread = thread;
		thread->start();
	}
}

void DeviceWidget::updateOutputTable(){
	//PVNLAB_DEBUG;
	if(m_tableOutputUpdateThread == NULL){
		TableUpdater *tabler = new TableUpdater(m_device, m_ui->tableWidgetOutputs, COLUMN_VAL);
		QThread* thread = new QThread();
		tabler->moveToThread(thread);
		connect(thread, SIGNAL(started()), tabler, SLOT(process()));
		connect(tabler, SIGNAL(finished()), thread, SLOT(quit()));
		connect(tabler, SIGNAL(finished()), tabler, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
		connect(tabler, SIGNAL(finished()), m_ui->tableWidgetOutputs->viewport(), SLOT(update()));	// serve per rinfrescare la visualizzazione dei dati
		connect(thread, SIGNAL(destroyed()), this, SLOT(tableOutputUpdateDone()));
		m_tableOutputUpdateThread = thread;
		thread->start();
	}
}

void DeviceWidget::updateDebugTable(){
	if(m_tableDebugUpdateThread == NULL){
		TableUpdater *tabler = new TableUpdater(m_device, m_ui->tableWidgetDebug, COLUMN_VAL);
		QThread* thread = new QThread();
		tabler->moveToThread(thread);
		connect(thread, SIGNAL(started()), tabler, SLOT(process()));
		connect(tabler, SIGNAL(finished()), thread, SLOT(quit()));
		connect(tabler, SIGNAL(finished()), tabler, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
		connect(tabler, SIGNAL(finished()), m_ui->tableWidgetDebug->viewport(), SLOT(update()));	// serve per rinfrescare la visualizzazione dei dati
		connect(thread, SIGNAL(destroyed()), this, SLOT(tableDebugUpdateDone()));
		m_tableDebugUpdateThread = thread;
		thread->start();
	}
}

void DeviceWidget::updatePersonalizedTable(){
	//PVNLAB_DEBUG;
	if(m_tablePersonalizedUpdateThread == NULL){
		TableUpdater *tabler = new TableUpdater(m_device, m_ui->tableWidgetPersonalized, COLUMN_VAL);
		QThread* thread = new QThread();
		tabler->moveToThread(thread);
		connect(thread, SIGNAL(started()), tabler, SLOT(process()));
		connect(tabler, SIGNAL(finished()), thread, SLOT(quit()));
		connect(tabler, SIGNAL(finished()), tabler, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
		connect(tabler, SIGNAL(finished()), m_ui->tableWidgetPersonalized->viewport(), SLOT(update()));	// serve per rinfrescare la visualizzazione dei dati
		connect(thread, SIGNAL(destroyed()), this, SLOT(tablePersonalizedUpdateDone()));
		m_tablePersonalizedUpdateThread = thread;
		thread->start();
	}
}

void DeviceWidget::tableRegisterUpdateDone(){
	//PVNLAB_DEBUG;
	m_tableRegisterUpdateThread = NULL;
}

void DeviceWidget::tableInputUpdateDone(){
	//PVNLAB_DEBUG;
	m_tableInputUpdateThread = NULL;
}

void DeviceWidget::tableOutputUpdateDone(){
	//PVNLAB_DEBUG;
	m_tableOutputUpdateThread = NULL;
}

void DeviceWidget::tablePersonalizedUpdateDone(){
	//PVNLAB_DEBUG;
	m_tablePersonalizedUpdateThread = NULL;
}

void DeviceWidget::tableDebugUpdateDone(){
	//PVNLAB_DEBUG;
	m_tableDebugUpdateThread = NULL;
}

void DeviceWidget::updateAlerts(){
	//PVNLAB_DEBUG;
	m_ui->listWidgetAlerts->clear();
	bool switcher = false;
	foreach(QString fail, m_device->activeFailures()){
		QListWidgetItem *item = new QListWidgetItem();
		item->setText(fail);
		if(switcher){
			if(m_darkmode){
				item->setForeground(PVNLAB_RED_2);
			}
			else{
				item->setBackground(PVNLAB_RED_2);
			}
		}
		else{
			if(m_darkmode){
				item->setForeground(PVNLAB_RED_1);
			}
			else{
				item->setBackground(PVNLAB_RED_1);
			}
		}
		m_ui->listWidgetAlerts->addItem(item);
		switcher = not switcher;
	}
	foreach(QString warn, m_device->activeWarnings()){
		QListWidgetItem *item = new QListWidgetItem();
		item->setText(warn);
		if(switcher){
			if(m_darkmode){
				item->setForeground(PVNLAB_YELLOW_2);
			}
			else{
				item->setBackground(PVNLAB_YELLOW_2);
			}
		}
		else{
			if(m_darkmode){
				item->setForeground(PVNLAB_YELLOW_1);
			}
			else{
				item->setBackground(PVNLAB_YELLOW_1);
			}
		}
		m_ui->listWidgetAlerts->addItem(item);
		switcher = not switcher;
	}
}

void DeviceWidget::updateStatus(){
	//PVNLAB_DEBUG;
	if(m_device->containsTag("RUNNING")){
		if(m_device->byTag("RUNNING")->boolValue()){
			m_ui->labelRunning->setPalette(QPalette(PVNLAB_GREEN_1));
		}
		else{
			m_ui->labelRunning->setPalette(QPalette(PVNLAB_GRAY_1));
		}
	}
	if(m_device->containsTag("TEST_MODE")){
		if(m_device->byTag("TEST_MODE")->boolValue()){
			m_ui->labelTestMode->setPalette(QPalette(PVNLAB_CYAN_2));
			m_ui->labelTestMode2->setPalette(QPalette(PVNLAB_CYAN_2));
		}
		else{
			m_ui->labelTestMode->setPalette(QPalette(PVNLAB_GRAY_1));
			m_ui->labelTestMode2->setPalette(QPalette(PVNLAB_GRAY_1));
		}
	}
	if(m_device->containsTag("WARNING")){
		if(m_device->byTag("WARNING")->boolValue()){
			m_ui->labelWarning->setPalette(QPalette(PVNLAB_YELLOW_1));
		}
		else{
			m_ui->labelWarning->setPalette(QPalette(PVNLAB_GRAY_1));
		}
	}
	if(m_device->containsTag("FAILURE")){
		if(m_device->byTag("FAILURE")->boolValue()){
			m_ui->labelFailure->setPalette(QPalette(PVNLAB_RED_1));
		}
		else{
			m_ui->labelFailure->setPalette(QPalette(PVNLAB_GRAY_1));
		}

	}
}

void DeviceWidget::updateTraceInfo(){
	//PVNLAB_DEBUG;
	if(m_device->containsAddress(m_ui->lineEditTraceAddress0->text())){
		m_ui->labelDesc0->setText(m_device->itemAt(m_ui->lineEditTraceAddress0->text())->description());
		m_ui->labelValue0->setText(m_device->itemAt(m_ui->lineEditTraceAddress0->text())->value());
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress1->text())){
		m_ui->labelDesc1->setText(m_device->itemAt(m_ui->lineEditTraceAddress1->text())->description());
		m_ui->labelValue1->setText(m_device->itemAt(m_ui->lineEditTraceAddress1->text())->value());
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress2->text())){
		m_ui->labelDesc2->setText(m_device->itemAt(m_ui->lineEditTraceAddress2->text())->description());
		m_ui->labelValue2->setText(m_device->itemAt(m_ui->lineEditTraceAddress2->text())->value());
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress3->text())){
		m_ui->labelDesc3->setText(m_device->itemAt(m_ui->lineEditTraceAddress3->text())->description());
		m_ui->labelValue3->setText(m_device->itemAt(m_ui->lineEditTraceAddress3->text())->value());
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress4->text())){
		m_ui->labelDesc4->setText(m_device->itemAt(m_ui->lineEditTraceAddress4->text())->description());
		m_ui->labelValue4->setText(m_device->itemAt(m_ui->lineEditTraceAddress4->text())->value());
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress5->text())){
		m_ui->labelDesc5->setText(m_device->itemAt(m_ui->lineEditTraceAddress5->text())->description());
		m_ui->labelValue5->setText(m_device->itemAt(m_ui->lineEditTraceAddress5->text())->value());
	}
}

void DeviceWidget::focusToFilter(){
	//PVNLAB_DEBUG;
	if(m_ui->tabWidget->currentIndex() == TAB_REG){
		m_ui->lineEditFilter->setFocus();
	}

}

void DeviceWidget::changeValue(){
	//PVNLAB_DEBUG;
	QTableWidget *table = NULL;
	if(m_ui->tabWidget->currentIndex() == TAB_REG){
		table = m_ui->tableWidgetRegisters;
	}
	if(m_ui->tabWidget->currentIndex() == TAB_PERSONAL){
		table = m_ui->tableWidgetPersonalized;
	}
	if(table != NULL){
		if(table->selectedItems().size() > 0){
			QTableWidgetItem *item = table->item(table->selectedItems()[0]->row(), COLUMN_VAL);
			changeValue(item);
		}
	}
}

void DeviceWidget::changeValue(QTableWidgetItem *twi){
	//PVNLAB_DEBUG;
	QString addr = addressAt(twi->tableWidget(), twi->row());
	if(m_device->containsAddress(addr)){
		PvnlabQModbusItem *item = m_device->itemAt(addr);
		if(item->isWritable()){
			if(item->isSerialNumberPart()){
				// il serial number non si può cambiare.
				;
			}
			else if(item->isMaterialNamePart()){
				bool ok;
				QString name = QInputDialog::getText(this, QCoreApplication::applicationName(), tr("Nome del materiale"), QLineEdit::Normal, m_device->materialName(), &ok);
				if(ok){
					m_device->setMaterialName(name);
					twi->tableWidget()->item(twi->row(), COLUMN_VAL)->setText("*" + name);
				}
			}
			else{
				if(item->type() != PvnlabQModbusItem::DIntL and item->type() != PvnlabQModbusItem::FloatL){
					InputDialog dialog(this, item);
					if(dialog.exec() == QDialog::Accepted){
						twi->tableWidget()->item(twi->row(), COLUMN_VAL)->setText("*" + dialog.tentativeValue());
					}
				}
			}
		}
	}
}

void DeviceWidget::sampleTimeChanged(int index){
	//PVNLAB_DEBUG;
	m_traceTimer->stop();
	m_traceTimer->setInterval(int(m_ui->comboBoxTraceSampleTime->itemText(index).toDouble() * 1000.0));
	m_traceTimer->start();
}

void DeviceWidget::updateTrace(){
	//PVNLAB_DEBUG;
	float v0 = 0, v1 = 0, v2 = 0, v3 = 0, v4 = 0, v5 = 0;
	if(m_device->containsAddress(m_ui->lineEditTraceAddress0->text())){
		v0 = m_device->itemAt(m_ui->lineEditTraceAddress0->text())->value().toFloat();
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress1->text())){
		v1 = m_device->itemAt(m_ui->lineEditTraceAddress1->text())->value().toFloat();
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress2->text())){
		v2 = m_device->itemAt(m_ui->lineEditTraceAddress2->text())->value().toFloat();
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress3->text())){
		v3 = m_device->itemAt(m_ui->lineEditTraceAddress3->text())->value().toFloat();
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress4->text())){
		v4 = m_device->itemAt(m_ui->lineEditTraceAddress4->text())->value().toFloat();
	}
	if(m_device->containsAddress(m_ui->lineEditTraceAddress5->text())){
		v5 = m_device->itemAt(m_ui->lineEditTraceAddress5->text())->value().toFloat();
	}
	if(m_traceX.size() > TRACE_MAX_SAMPLES){
		if(not m_traceX.isEmpty()){
			m_traceX.removeFirst();
			m_traceY[0].removeFirst();
			m_traceY[1].removeFirst();
			m_traceY[2].removeFirst();
			m_traceY[3].removeFirst();
			m_traceY[4].removeFirst();
			m_traceY[5].removeFirst();
		}
	}
	m_traceX.append(QwtDate::toDouble(QDateTime::currentDateTime()));
	m_traceY[0].append(v0);
	m_traceY[1].append(v1);
	m_traceY[2].append(v2);
	m_traceY[3].append(v3);
	m_traceY[4].append(v4);
	m_traceY[5].append(v5);
	replot();
}

void DeviceWidget::filterRegistersTable(QString text){
	//PVNLAB_DEBUG;
	for(int i = 0; i < m_ui->tableWidgetRegisters->rowCount(); i++){
		bool unmatched = (text != "") and not (m_ui->tableWidgetRegisters->item(i, COLUMN_DESC)->text().contains(text, Qt::CaseInsensitive));
		m_ui->tableWidgetRegisters->setRowHidden(i, unmatched);
	}
}

void DeviceWidget::setCurrentTab(int index){
	//PVNLAB_DEBUG;
	m_ui->tabWidget->setCurrentIndex(index);
}

void DeviceWidget::selectPrevRow(){
	//PVNLAB_DEBUG;
	QTableWidget *table = NULL;
	if(m_ui->tabWidget->currentIndex() == TAB_REG){
		table = m_ui->tableWidgetRegisters;
	}
	if(m_ui->tabWidget->currentIndex() == TAB_PERSONAL){
		table = m_ui->tableWidgetPersonalized;
	}

	if(table != NULL){
		int current = 1;
		if(table->selectedItems().size() > 0){
			current = table->selectedItems()[0]->row();
		}
		current--;
		if(current < 0){
			current = 0;
		}
		table->selectRow(current);
		table->scrollToItem(table->item(current,0));
	}
}

void DeviceWidget::selectNextRow(){
	//PVNLAB_DEBUG;
	QTableWidget *table = NULL;
	if(m_ui->tabWidget->currentIndex() == TAB_REG){
		table = m_ui->tableWidgetRegisters;
	}
	if(m_ui->tabWidget->currentIndex() == TAB_IO){
		table = m_ui->tableWidgetDebug;
	}
	if(m_ui->tabWidget->currentIndex() == TAB_PERSONAL){
		table = m_ui->tableWidgetPersonalized;
	}

	if(table != NULL){
		int current = -1;
		if(table->selectedItems().size() > 0){
			current = table->selectedItems()[0]->row();
		}
		current++;
		if(current >= table->rowCount()){
			current = table->rowCount() - 1;
		}
		table->selectRow(current);
		table->scrollToItem(table->item(current,0));
	}
}

void DeviceWidget::traceAddress(int index, QString address){
	//PVNLAB_DEBUG;

	if(index == 0){
		m_ui->lineEditTraceAddress0->setText(address);
	}
	else if(index == 1){
		m_ui->lineEditTraceAddress1->setText(address);
	}
	else if(index == 2){
		m_ui->lineEditTraceAddress2->setText(address);
	}
	else if(index == 3){
		m_ui->lineEditTraceAddress3->setText(address);
	}
	else if(index == 4){
		m_ui->lineEditTraceAddress4->setText(address);
	}
	else if(index == 5){
		m_ui->lineEditTraceAddress5->setText(address);
	}
	updateTraceInfo();

}

void DeviceWidget::showCustomMenuMain(QPoint pos){
	//PVNLAB_DEBUG;
	showCustomMenu(m_ui->tableWidgetRegisters, pos);
}

void DeviceWidget::showCustomMenuInputs(QPoint pos){
	//PVNLAB_DEBUG;
	showCustomMenu(m_ui->tableWidgetInputs, pos);
}

void DeviceWidget::showCustomMenuOutputs(QPoint pos){
	//PVNLAB_DEBUG;
	showCustomMenu(m_ui->tableWidgetOutputs, pos);
}

void DeviceWidget::showCustomMenuDebug(QPoint pos){
	//PVNLAB_DEBUG;
	showCustomMenu(m_ui->tableWidgetDebug, pos);
}

void DeviceWidget::showCustomMenuPersonalized(QPoint pos){
	//PVNLAB_DEBUG;
	showCustomMenu(m_ui->tableWidgetPersonalized, pos);
}

void DeviceWidget::showCustomMenu(QTableWidget *table, QPoint pos){
	//PVNLAB_DEBUG;
	QMenu menu(this);
	QAction *trace0 = menu.addAction(tr("Traccia (ciano)"));
	QAction *trace1 = menu.addAction(tr("Traccia (giallo)"));
	QAction *trace2 = menu.addAction(tr("Traccia (rosso)"));
	QAction *trace3 = menu.addAction(tr("Traccia (verde)"));
	QAction *trace4 = menu.addAction(tr("Traccia (arancio)"));
	QAction *trace5 = menu.addAction(tr("Traccia (fuxia)"));
	QAction *addToPersonalizedTable = menu.addAction(tr("Aggiungi alla tabella personalizzata"));
	addToPersonalizedTable->setEnabled(not m_logging and (table != m_ui->tableWidgetPersonalized));
	QAction *a = menu.exec(table->viewport()->mapToGlobal(pos));
	QString address = addressAt(table, table->itemAt(pos)->row());

	if (a == trace0){
		traceAddress(0, address);
	}
	else if (a == trace1){
		traceAddress(1, address);
	}
	else if (a == trace2){
		traceAddress(2, address);
	}
	else if (a == trace3){
		traceAddress(3, address);
	}
	else if (a == trace4){
		traceAddress(4, address);
	}
	else if (a == trace5){
		traceAddress(5, address);
	}
	else if(a == addToPersonalizedTable){
		addToPersonalized(address);
	}
}

bool DeviceWidget::addToPersonalized(QString address){
	//PVNLAB_DEBUG;
	bool done = false;
	if(m_device->containsAddress(address) and not m_personalizedAddresses.contains(address)){
		m_personalizedAddresses<<address;
		qSort(m_personalizedAddresses);
		initializeTable(m_ui->tableWidgetPersonalized, m_personalizedAddresses);
		done = true;
	}
	return done;
}

void DeviceWidget::clearPersonalized(){
	//PVNLAB_DEBUG;
	m_personalizedAddresses.clear();
	initializeTable(m_ui->tableWidgetPersonalized, m_personalizedAddresses);
}

void DeviceWidget::addToPersonalized(){
	//PVNLAB_DEBUG;
	bool ok;
	QString addr = QInputDialog::getText(this, QCoreApplication::applicationName(), "Indirizzo",QLineEdit::Normal, "", &ok);
	if(ok){
		addToPersonalized(addr);
	}

}

void DeviceWidget::removeFromPersonalized(){
	//PVNLAB_DEBUG;
	if(m_ui->tableWidgetPersonalized->selectedItems().size() > 0){
		int row = m_ui->tableWidgetPersonalized->selectedItems()[0]->row();
		QString addr = addressAt(m_ui->tableWidgetPersonalized, row);
		m_personalizedAddresses.removeOne(addr);
		m_ui->tableWidgetPersonalized->removeRow(row);
	}

}

void DeviceWidget::recLog(){
	//PVNLAB_DEBUG;
	if(not m_personalizedAddresses.isEmpty() and not m_logging){
		QString path = QFileDialog::getSaveFileName(this, QCoreApplication::applicationName() + tr(" - File di log"),  m_logDir, tr("CSV file (*.csv)"));
		QFileInfo finfo(path);
		if(path != ""){
			m_logDir = finfo.absolutePath();
			prepareRecording(finfo.absoluteFilePath());
			recNow();
			connect(m_logTimer, SIGNAL(timeout()), this, SLOT(recNow()));
			m_logTimer->start(int(m_ui->comboBoxRecSampleTime->currentText().toDouble() * 1000.0));
			m_logging = true;
		}
	}
}

void DeviceWidget::prepareRecording(QString path){
	//PVNLAB_DEBUG;
	m_logPath = path;
	QString line;
	QTextStream out(&line);
	out.setCodec("UTF-8");
	out<<QDate::currentDate().toString("yyyy-MM-dd")<<";";
	for(int i = 0; i < m_personalizedAddresses.length(); i++){
		out<<m_personalizedAddresses[i] + " - " + m_device->itemAt(m_personalizedAddresses[i])->description() + ";";
	}
	if(!Logger::log(m_logPath, line)){
		QMessageBox::information(this, QCoreApplication::applicationName() + tr(" - Errore nei log"), tr("Impossibile scrivere il file di log:") + m_logPath);
	}
	m_logCount = 0;
	m_ui->labelRec->setText("REC[" + QString::number(m_logCount) + "]");

	m_ui->pushButtonPersonalizedAdd->setEnabled(false);
	m_ui->pushButtonPersonalizedAddAll->setEnabled(false);
	m_ui->pushButtonPersonalizedClear->setEnabled(false);
	m_ui->pushButtonPersonalizedRemove->setEnabled(false);
	m_ui->comboBoxRecSampleTime->setEnabled(false);

}

void DeviceWidget::recNow(){
	//PVNLAB_DEBUG;
	QString line;
	QTextStream out(&line);
	out.setCodec("UTF-8");
	out<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")<<";";
	for(int i = 0; i < m_personalizedAddresses.length(); i++){
		out<<m_device->itemAt(m_personalizedAddresses[i])->value() + ";";
	}
	if(!Logger::log(m_logPath, line)){
		QMessageBox::information(this, QCoreApplication::applicationName() + tr(" - Errore nei log"), tr("Impossibile scrivere il file di log:") + m_logPath);
	}
	m_logCount++;
	m_ui->labelRec->setText("REC[" + QString::number(m_logCount) + "]");
	m_ui->labelRec->setPalette(QPalette(PVNLAB_RED_1));
}

void DeviceWidget:: stopLog(){
	//PVNLAB_DEBUG;
	if(m_logging){
		m_logTimer->stop();
		disconnect(m_logTimer, SIGNAL(timeout()), this, SLOT(recNow()));
		m_ui->labelRec->setPalette(QPalette(PVNLAB_GRAY_2));
		m_logging = false;
		m_ui->pushButtonPersonalizedAdd->setEnabled(true);
		m_ui->pushButtonPersonalizedAddAll->setEnabled(true);
		m_ui->pushButtonPersonalizedClear->setEnabled(true);
		m_ui->pushButtonPersonalizedRemove->setEnabled(true);
		m_ui->comboBoxRecSampleTime->setEnabled(true);
	}
}

void DeviceWidget::addAllToPersonalized(){
	//PVNLAB_DEBUG;
	m_personalizedAddresses.clear();
	foreach(QString addr, m_device->sortedAddresses()){
		m_personalizedAddresses<<addr;
	}
	qSort(m_personalizedAddresses);
	initializeTable(m_ui->tableWidgetPersonalized, m_personalizedAddresses);
}

void DeviceWidget::saveRegisters(){
	//PVNLAB_DEBUG;
	QString path = QFileDialog::getSaveFileName(this, QCoreApplication::applicationName() + tr(" - Salva dati"), m_registersDir , tr("File di dati di DruidLab (*.shd)"));
	QFileInfo finfo(path);
	if(path != ""){
		m_registersDir = finfo.absolutePath();
		PvnlabQModbusDevice::Error err = m_device->saveRegisters(path);
		if(err != PvnlabQModbusDevice::NoError){
			QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Errore di salvataggio dei dati"));
		}
	}

}

void DeviceWidget::uploadRegisters(){
	//PVNLAB_DEBUG;
	QString path = QFileDialog::getOpenFileName(this, QCoreApplication::applicationName() + tr(" - Carica dati"), m_registersDir, tr("File di dati di DruidLab (*.shd)"));
	QFileInfo finfo(path);
	if(path != ""){
		m_registersDir = finfo.absolutePath();
		PvnlabQModbusDevice::Error err = m_device->uploadRegisters(path);
		if(err != PvnlabQModbusDevice::NoError){
			QString second = "";
			if(err == PvnlabQModbusDevice::DeviceCompatError){
				second = tr("Dispositivo non compatibile.");
			}
			else if(err == PvnlabQModbusDevice::DeviceCompatError){
				second = tr("Impossibile aprire il file.");
			}
			QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Errore caricamento dati.") + "\n" + second);
		}

	}
}

void DeviceWidget::clearPlot(){
	//PVNLAB_DEBUG;
	m_ui->qwtPlot->detachItems(QwtPlotItem::Rtti_PlotCurve, true);
	m_ui->qwtPlot->detachItems(QwtPlotItem::Rtti_PlotGrid, true);
	m_ui->qwtPlot->replot();
}

void DeviceWidget::clearTraces(){
	//PVNLAB_DEBUG;
	m_traceX.clear();
	for(int i = 0; i < 6; i++){
		m_traceY[i].clear();
	}
	clearPlot();
}

void DeviceWidget::replot(){
	//PVNLAB_DEBUG;
	if(m_plotZoomer->zoomRect() == m_plotZoomer->zoomBase()){
		m_ui->labelTraceZoomStatus->setText(tr("Zoom non attivo"));
		clearPlot();
		QwtPlotGrid *grid = new QwtPlotGrid();
		grid->setMajorPen(QColor(64, 64, 64), 1.0, Qt::DashLine);
		grid->setMinorPen(QColor(32, 32, 32), 1.0, Qt::DotLine);
		grid->enableXMin(true);
		grid->enableYMin(true);
		grid->enableX(true);
		grid->enableY(true);
		grid->attach((m_ui->qwtPlot));

		int scales[6];
		scales[0] = m_ui->comboBoxTrace0->currentIndex();
		scales[1] = m_ui->comboBoxTrace1->currentIndex();
		scales[2] = m_ui->comboBoxTrace2->currentIndex();
		scales[3] = m_ui->comboBoxTrace3->currentIndex();
		scales[4] = m_ui->comboBoxTrace4->currentIndex();
		scales[5] = m_ui->comboBoxTrace5->currentIndex();
		QString descs[6];
		descs[0] = m_ui->labelDesc0->text();
		descs[1] = m_ui->labelDesc1->text();
		descs[2] = m_ui->labelDesc2->text();
		descs[3] = m_ui->labelDesc3->text();
		descs[4] = m_ui->labelDesc4->text();
		descs[5] = m_ui->labelDesc5->text();

		for(int i = 0; i < 6; i++){
			if(scales[i] != 0){
				QwtPlotCurve* curve =  new QwtPlotCurve(descs[i]);
				if(scales[i] == 1){
					curve->setPen(m_traceColors[i], 1.0, Qt::SolidLine);
					curve->setAxes(QwtPlot::xBottom, QwtPlot::yLeft);
				}
				else if(scales[i] == 2){
					curve->setPen(m_traceColors[i], 1.0, Qt::DotLine);
					curve->setAxes(QwtPlot::xBottom, QwtPlot::yRight);
				}
				curve->setStyle(QwtPlotCurve::Lines);
				curve->setSamples(m_traceX, m_traceY[i]);
				curve->attach(m_ui->qwtPlot);
				//m_ui->qwtPlot->setVisible(true);
				m_ui->qwtPlot->replot();
				updatePlotScales();
			}
		}
	}
}

void DeviceWidget::updatePlotScales(){
	//PVNLAB_DEBUG;
	// BOTTOM
	// non so perchè non funzionava in automatico ma così va ben
	//m_ui->qwtpPlot->setAxisAutoScale(QwtPlot::xBottom, true);
	m_ui->qwtPlot->setAxisAutoScale(QwtPlot::xBottom, false);
	qint64 seconds[] = {-60, -600, -1800, -3600, -7200, -14400, -28800, -86400};

	m_ui->qwtPlot->setAxisScale(QwtPlot::xBottom, QwtDate::toDouble(QDateTime::currentDateTime().addSecs(seconds[m_ui->comboBoxTraceSpanTime->currentIndex()])),
			    QwtDate::toDouble(QDateTime::currentDateTime()));

	// LEFT
	m_ui->qwtPlot->setAxisAutoScale(QwtPlot::yRight, false);
	if(true){
		m_ui->qwtPlot->setAxisScale(QwtPlot::yLeft, m_ui->lineEditMin1->text().toDouble(), m_ui->lineEditMax1->text().toDouble());
		m_ui->qwtPlot->updateAxes();
	}
	else{
		m_ui->qwtPlot->updateAxes();
		m_ui->lineEditMin1->setText(QString::number(m_ui->qwtPlot->axisScaleDiv(QwtPlot::yLeft).lowerBound()));
		m_ui->lineEditMax1->setText(QString::number(m_ui->qwtPlot->axisScaleDiv(QwtPlot::yLeft).upperBound()));
	}

	// RIGHT
	m_ui->qwtPlot->setAxisAutoScale(QwtPlot::yRight, false);
	if(true){
		m_ui->qwtPlot->setAxisScale(QwtPlot::yRight, m_ui->lineEditMin2->text().toDouble(), m_ui->lineEditMax2->text().toDouble());
		m_ui->qwtPlot->updateAxes();
	}
	else{
		m_ui->qwtPlot->updateAxes();
		m_ui->lineEditMin2->setText(QString::number(m_ui->qwtPlot->axisScaleDiv(QwtPlot::yLeft).lowerBound()));
		m_ui->lineEditMax2->setText(QString::number(m_ui->qwtPlot->axisScaleDiv(QwtPlot::yLeft).upperBound()));
	}
	m_plotZoomer->setZoomBase();
}

void DeviceWidget::resetZoom(){
	//PVNLAB_DEBUG;
	m_plotZoomer->setZoomBase();
	m_ui->labelTraceZoomStatus->setText(tr("Zoom non attivo"));
	m_ui->labelTraceZoomStatus->setAutoFillBackground(false);
	replot();
}

void DeviceWidget::traceZoomed(){
	//PVNLAB_DEBUG;
	if(m_plotZoomer->zoomRect() == m_plotZoomer->zoomBase()){
		m_ui->labelTraceZoomStatus->setText(tr("Zoom non attivo"));
		m_ui->labelTraceZoomStatus->setAutoFillBackground(false);
	}
	else{
		m_ui->labelTraceZoomStatus->setText(tr("Zoom attivo. Aggiornamento del grafico interrotto"));
		m_ui->labelTraceZoomStatus->setAutoFillBackground(true);
		//QPalette red(PVNLAB_RED_1);
		m_ui->labelTraceZoomStatus->setPalette(QPalette(PVNLAB_RED_1));

	}

}

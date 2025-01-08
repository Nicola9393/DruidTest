/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#define COLUMN_DESC 2
#define COLUMN_VAL 3
#define TAB_REG 0
#define TAB_TRACE 1
#define TAB_PERSONAL 2
#define TAB_IO 3
#define TAB_ALERT 4
#define TRACE_MAX_SAMPLES  17280

#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE

class QTableWidgetItem;
class QTableWidget;
class QLabel;
class TraceWidget;
class QMenu;
class QShortcut;

namespace Ui {
	class DeviceWidget;
}

class PvnlabQModbusDevice;
class TableUpdater;
class QwtPlotCurve;
class QwtPlotZoomer;
class QwtDateScaleDraw;
class QwtLegend;
class QwtPlotPicker;

QT_END_NAMESPACE

class DeviceWidget : public QWidget
{
	Q_OBJECT

	friend class TableUpdater;

public:
	DeviceWidget(QWidget *parent, PvnlabQModbusDevice *device, QString cfg, bool darmkode);
	~DeviceWidget();

	static QString addressAt(QTableWidget *table, int row);
	void setCurrentTab(int index);



public slots:
	void setVisible(bool visible);
	void update();
	void focusToFilter();

private slots:
	void changeValue();
	void changeValue(QTableWidgetItem *twi);
	void sampleTimeChanged(int index);
	void updateTrace();
	void filterRegistersTable(QString);
	void selectPrevRow();
	void selectNextRow();
	void showCustomMenuMain(QPoint pos);
	void showCustomMenuInputs(QPoint pos);
	void showCustomMenuOutputs(QPoint pos);
	void showCustomMenuDebug(QPoint pos);
	void showCustomMenuPersonalized(QPoint pos);
	void clearPersonalized();
	void addToPersonalized();
	void removeFromPersonalized();
	void recLog();
	void stopLog();
	void addAllToPersonalized();
	void prepareRecording(QString path);
	void recNow();
	void saveRegisters();
	void uploadRegisters();
	void tableRegisterUpdateDone();
	void tableInputUpdateDone();
	void tableOutputUpdateDone();
	void tablePersonalizedUpdateDone();
	void tableDebugUpdateDone();
	void clearPlot();
	void clearTraces();
	void replot();
	void updatePlotScales();
	void resetZoom();
	void traceZoomed();

signals:

private:
	void readSettings();
	void saveSettings();
	void initialize();
	void initializeTable(QTableWidget *table, QStringList addresses);
	void updateTraceInfo();
	void showCustomMenu(QTableWidget *table, QPoint pos);
	void traceAddress(int index, QString address);
	bool addToPersonalized(QString address);
	void updateAlerts();
	void updateStatus();
	void reallyQuitTableThread(QThread *thread);
	void updateRegisterTable();
	void updateInputTable();
	void updateOutputTable();
	void updateDebugTable();
	void updatePersonalizedTable();




	Ui::DeviceWidget *m_ui;
	PvnlabQModbusDevice *m_device;
	QString m_cfg;
	//TraceWidget *m_trace;
	QTimer *m_traceTimer;
	QShortcut *m_recListDeleteShortcut;
	QShortcut *m_clearFilterShortcut;
	QShortcut *m_upShortcut;
	QShortcut *m_downShortcut;
	QStringList m_personalizedAddresses;
	QString m_logPath;
	QString m_registersDir;
	bool m_logging;
	QString m_logDir;
	QTimer *m_logTimer;
	quint32 m_logCount;

	QThread* m_tableInputUpdateThread;
	QThread* m_tableOutputUpdateThread;
	QThread* m_tableDebugUpdateThread;
	QThread* m_tableRegisterUpdateThread;
	QThread* m_tablePersonalizedUpdateThread;

	QwtPlotZoomer *m_plotZoomer;
	QwtDateScaleDraw *m_plotDateDraw;
	QwtLegend *m_plotLegend;
	QwtPlotPicker *m_plotPicker;

	QVector<double> m_traceX;
	QVector<double> m_traceY[6];
	QColor m_traceColors[6];

	bool m_darkmode;


};

#endif // GMPDEVICEWIDGET_H

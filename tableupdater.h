/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#ifndef TABLEUPDATER_H
#define TABLEUPDATER_H

#include <QObject>

QT_BEGIN_NAMESPACE

class QTableWidget;
class PvnlabQModbusDevice;

QT_END_NAMESPACE

class TableUpdater : public QObject
{
	Q_OBJECT
public:
	explicit TableUpdater(PvnlabQModbusDevice *device, QTableWidget *m_table, int column);

signals:
	void finished();

public slots:
	void process();

private:
	PvnlabQModbusDevice *m_device;
	QTableWidget *m_table;
	int m_column;

};

#endif // TABLEUPDATER_H

/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#include "tableupdater.h"

#include "QTableWidget"
#include "pvnlab_qmodbus_item.h"
#include "pvnlab_qmodbus_device.h"
#include "devicewidget.h"
#include "pvnlab.h"

TableUpdater::TableUpdater(PvnlabQModbusDevice *device, QTableWidget *table, int column) {
	m_device = device;
	//PVNLAB_DEBUG;
	m_table = table;
	m_column = column;

	typedef QVector<QVector<int> > FakeArray;
	qRegisterMetaType<FakeArray>("FakeArray");
	/*
	queste due righe servono per non avere l'errore
	QObject::connect: Cannot queue arguments of type 'QVector<int>'
	(Make sure 'QVector<int>' is registered using qRegisterMetaType().)
	*/
}

void TableUpdater::process(){
	//PVNLAB_DEBUG;
	for(int i = 0; i < m_table->rowCount(); i++){
		QTableWidgetItem *cell = m_table->item(i, m_column);
		QString addr = DeviceWidget::addressAt(m_table, i);
		if(m_device->containsAddress(addr)){
			PvnlabQModbusItem *item = m_device->itemAt(addr);
			if(item->errcode() == 0){
				if(item->isSerialNumberPart()){
					if(item->type() == PvnlabQModbusItem::Sn3){
						cell->setText(m_device->serialNumber());
					}
					else{
						cell->setText("-");
					}
				}
				else if(item->isMaterialNamePart()){
					if(item->type() == PvnlabQModbusItem::Mat9){
						cell->setText(m_device->materialName());
					}
					else{
						cell->setText("-");
					}
				}
				else{
					cell->setText(item->value());
				}
			}
			else{
				cell->setText("# ERR: " + QString::number(item->errcode()));
			}
		}
	}
	emit finished();

}


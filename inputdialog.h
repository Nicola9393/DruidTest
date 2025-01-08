/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui {
	class InputDialog;
}

class PvnlabQModbusItem;

QT_END_NAMESPACE

class InputDialog : public QDialog
{
	Q_OBJECT

public:
	InputDialog(QWidget *parent, PvnlabQModbusItem *item);
	~InputDialog();

	QString tentativeValue();

protected:
	void accept();
	void reject();

private slots:
	void doubleValueChanged(double value);

private:
	Ui::InputDialog *m_ui;

	PvnlabQModbusItem *m_item;
	PvnlabQModbusItem *m_maxItem;
	PvnlabQModbusItem *m_minItem;
	QString m_tentativeValue;
};

#endif // INPUTDIALOG_H

/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#include "inputdialog.h"
#include "ui_inputdialog.h"
#include "pvnlab.h"
#include "pvnlab_qmodbus_item.h"
#include "math.h"
#include <limits>
#include <QShortcut>

InputDialog::InputDialog(QWidget *parent, PvnlabQModbusItem *item): QDialog(parent), m_ui(new Ui::InputDialog){
	//PVNLAB_DEBUG;
	m_ui->setupUi(this);

	connect(m_ui->dsbNumber, SIGNAL(valueChanged(double)), this, SLOT(doubleValueChanged(double)));

	m_item = item;
	m_ui->lbDescription->setText(item->description());

	if(m_item->type() == PvnlabQModbusItem::Bit){
		m_ui->cbInput->setVisible(false);
		m_ui->frNumber->setVisible(false);
		m_ui->frText->setVisible(false);
		m_ui->frBoolean->setVisible(true);
		m_ui->rbTrue->setChecked(m_item->boolValue());
		m_ui->rbFalse->setChecked(not m_item->boolValue());
		m_ui->rbTrue->setFocus();
	}
	else if(m_item->type() == PvnlabQModbusItem::Combo){
		m_ui->cbInput->setVisible(true);
		m_ui->frNumber->setVisible(false);
		m_ui->frText->setVisible(false);
		m_ui->frBoolean->setVisible(false);
		m_ui->cbInput->addItems(m_item->choices());
		m_ui->cbInput->setCurrentIndex(m_item->choices().indexOf(m_item->value()));
		m_ui->cbInput->setFocus();
	}
	else if(m_item->type() == PvnlabQModbusItem::Int or m_item->type() == PvnlabQModbusItem::UInt or
			m_item->type() == PvnlabQModbusItem::DIntM or m_item->type() == PvnlabQModbusItem::DIntL or
			m_item->type() == PvnlabQModbusItem::Word or
			m_item->type() == PvnlabQModbusItem::FloatL or m_item->type() == PvnlabQModbusItem::FloatM){
		m_ui->cbInput->setVisible(false);
		m_ui->frNumber->setVisible(true);
		m_ui->frBoolean->setVisible(false);
		m_ui->frText->setVisible(false);

		if(m_item->type() == PvnlabQModbusItem::Word or m_item->type() == PvnlabQModbusItem::UInt){
			m_ui->dsbNumber->setRange(0, std::numeric_limits<int>::max());
		}
		else{
			m_ui->dsbNumber->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
		}
		if(m_item->type() == PvnlabQModbusItem::FloatL or m_item->type() == PvnlabQModbusItem::FloatM){
			m_ui->dsbNumber->setDecimals(6);
			m_ui->dsbNumber->setSingleStep(1);
		}
		else{
			if(m_item->factor() < 0){
				m_ui->dsbNumber->setDecimals(-m_item->factor());
				m_ui->dsbNumber->setSingleStep(pow(10.0, (double)(m_item->factor())));
			}
			else{
				m_ui->dsbNumber->setDecimals(0);
				m_ui->dsbNumber->setSingleStep(1);
			}
		}
		m_ui->lbUnit->setText(m_item->unit());
		//if(m_item->intMin() != 0 or m_item->intMax() != 0){
		if(m_item->min().toDouble() != m_item->max().toDouble()){
			m_ui->lbDescription->setText(item->description() + "\n[ " + m_item->min() + " ... " + m_item->max() + " ]");
		}
		m_ui->dsbNumber->setValue(m_item->value().toDouble());
		m_ui->dsbNumber->selectAll();

	}
	else{
		m_ui->cbInput->setVisible(false);
		m_ui->frNumber->setVisible(false);
		m_ui->frBoolean->setVisible(false);
		m_ui->frText->setVisible(true);
		m_ui->leInputText->setText(m_item->value());
		m_ui->leInputText->setSelection(0, m_item->value().length());
		m_ui->leInputText->setFocus();

	}

	new QShortcut(QKeySequence(Qt::Key_1), m_ui->rbTrue, SLOT(click()));
	new QShortcut(QKeySequence(Qt::Key_0), m_ui->rbFalse, SLOT(click()));


	adjustSize();
	setWindowTitle(QCoreApplication::applicationName());
}

void InputDialog::accept(){
	if(m_ui->frBoolean->isVisible()){
		if(m_ui->rbTrue->isChecked()){
			m_item->setValue(true);
			m_tentativeValue = VAL_TRUE;
		}
		else{
			m_item->setValue(false);
			m_tentativeValue = VAL_FALSE;
		}
	}
	else if(m_ui->frText->isVisible()){
		m_item->setValue(m_ui->leInputText->text());
		m_tentativeValue = m_ui->leInputText->text();
	}
	else if(m_ui->cbInput->isVisible()){
		m_item->setValue(m_ui->cbInput->currentText());
		m_tentativeValue = m_ui->cbInput->currentText();
	}
	else if(m_ui->frNumber->isVisible()){
		m_item->setValue(m_ui->dsbNumber->text());
		m_tentativeValue = m_ui->dsbNumber->text();
	}
	//qApp->processEvents();			// se non lo metto arriva un SEGFAULT ... boh ...
	QDialog::accept();

}

void InputDialog::reject(){
	//PVNLAB_DEBUG;
	QDialog::reject();
}

InputDialog::~InputDialog(){
	//PVNLAB_DEBUG;
	delete m_ui;
}

QString InputDialog::tentativeValue(){
	//PVNLAB_DEBUG;
	return m_tentativeValue;
}

void InputDialog::doubleValueChanged(double value){
	//PVNLAB_DEBUG;
	//if(m_item->intMin() != 0 or m_item->intMax() != 0){
	double min = m_item->min().toDouble();
	double max = m_item->max().toDouble();
	if(min != max){
		if(value < min){
			QPalette pal(m_ui->dsbNumber->palette());
			pal.setColor(QPalette::Active, QPalette::Base, PVNLAB_YELLOW_1);
			m_ui->dsbNumber->setPalette(pal);
		}
		else if(value > max){
			QPalette pal(m_ui->dsbNumber->palette());
			pal.setColor(QPalette::Active, QPalette::Base, PVNLAB_RED_1);
			m_ui->dsbNumber->setPalette(pal);
		}
		else{
			QPalette pal(m_ui->dsbNumber->palette());
			pal.setColor(QPalette::Active, QPalette::Base, Qt::white);
			m_ui->dsbNumber->setPalette(pal);
		}
	}
}


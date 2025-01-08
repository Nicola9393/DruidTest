/****************************************************************************
**
** Copyright (C) 2013 Piovan S.p.a
**
**	Author: Enricomaria Pavan
**
**
****************************************************************************/

#include "tracewidget.h"
#include "ui_tracewidget.h"
#include <QPainter>
#include <QDebug>
#include "pvnlab.h"

TraceWidget::TraceWidget(QWidget *parent) : QWidget(parent), ui(new Ui::TraceWidget)
{
	//PVNLAB_DEBUG;
	ui->setupUi(this);
	this->setAutoFillBackground(true);

	m_colors[0] = PVNLAB_NEON_CYAN;
	m_colors[1] = PVNLAB_NEON_YELLOW;
	m_colors[2] = PVNLAB_NEON_RED;
	m_colors[3] = PVNLAB_NEON_GREEN;
	m_colors[4] = PVNLAB_NEON_ORANGE;
	m_colors[5] = PVNLAB_NEON_FUCHSIA;

	clear();
	setScale1(0, 500);
}

TraceWidget::~TraceWidget()
{
	//PVNLAB_DEBUG;
	delete ui;
}

void TraceWidget::paintEvent(QPaintEvent* event){
	//PVNLAB_DEBUG;
	QPainter painter(this);
	QFont font = painter.font();
	font.setPointSize(8);
	painter.setFont(font);

	drawXAxis(painter);
	drawYAxis1(painter);
	drawYAxis2(painter);

	for(int i = 0 ; i < TRACE_COUNT; i++){
		drawFifo(i, painter);
	}

}

void TraceWidget::clear(){
	//PVNLAB_DEBUG;
	for (int i = 0; i < TRACE_COUNT; i ++){
		for (int j = 0; j < TRACE_FIFO_SIZE; j ++){
			m_fifo[i][j] = 0;
		}
	}
	m_count = 0;
	this->repaint();
}
void TraceWidget::setScale1(float min, float max){
	//PVNLAB_DEBUG;
	m_min1 = min;
	m_max1 = max;
	float step = (max - min) / 10.0;
	for (int i = 0; i < 11; i ++){
		m_ticks1[i] = min + (step * i);
	}
	this->repaint();
}

void TraceWidget::setScale2(float min, float max){
	//PVNLAB_DEBUG;
	m_min2 = min;
	m_max2 = max;
	float step = (max - min) / 10.0;
	for (int i = 0; i < 11; i ++){
		m_ticks2[i] = min + (step * i);
	}
	this->repaint();
}

void TraceWidget::push(float v0, float v1, float v2, float v3, float v4, float v5){
	//PVNLAB_DEBUG;
	// sposta valori indietro
	for(int i = 0; i < TRACE_COUNT; i++){
		for(int j = 99; j > 0 ; j--){
			m_fifo[i][j] = m_fifo[i][j - 1];
		}
	}
	// aggiungi nuovi valori
	m_fifo[0][0] = v0;
	m_fifo[1][0] = v1;
	m_fifo[2][0] = v2;
	m_fifo[3][0] = v3;
	m_fifo[4][0] = v4;
	m_fifo[5][0] = v5;
	if(m_count < TRACE_FIFO_SIZE){
		m_count++;
	}

	this->repaint();
}

void TraceWidget::drawXAxis(QPainter &painter){
	//PVNLAB_DEBUG;
	painter.setPen(Qt::white);
	painter.drawLine(4,this->height() - 30, this->width() - 4, this->height() - 30);
	float step = (this->width() - 30.0) / 10.0;
	for(int i = 1; i < 10; i++){
		painter.drawLine( 30.0 + (i * step), this->height() - 26.0,  30.0 + (i * step), this->height() - 30.0);
		//painter.drawText( 30.0 + (i * step) - 8.0, this->height() - 4.0, QString::number(m_ticks[i]));
	}
	QPen pen(Qt::DotLine);
	pen.setColor(Qt::darkGray);
	painter.setPen(pen);
	for(int i = 1; i < 10; i++){
		painter.drawLine( 30.0 + (i * step), this->height() - 31.0,  30.0 + (i * step), 0.0);
	}
}

void TraceWidget::drawYAxis1(QPainter &painter){
	//PVNLAB_DEBUG;
	painter.setPen(Qt::white);
	painter.drawLine(30, 4, 30, this->height() - 4);
	float step = (this->height() - 30.0) / 10.0;
	for(int i = 1; i < 10; i++){
		painter.drawLine(26.0, (this->height() - 30.0) - (i * step) , 30.0, (this->height() - 30.0) - (i * step));
		painter.drawText(4.0, (this->height() - 30.0) - (i * step) + 4, QString::number(m_ticks1[i]));
	}
	QPen pen(Qt::DotLine);
	pen.setColor(Qt::darkGray);
	painter.setPen(pen);
	for(int i = 1; i < 10; i++){
		painter.drawLine(31.0, (this->height() - 30.0) - (i * step) , this->width() - 31.0, (this->height() - 30.0) - (i * step));
	}
}

void TraceWidget::drawYAxis2(QPainter &painter){
	//PVNLAB_DEBUG;
	painter.setPen(Qt::white);
	painter.drawLine(this->width() - 30, 4, this->width() -30, this->height() - 4);
	float step = (this->height() - 30.0) / 10.0;
	for(int i = 1; i < 10; i++){
		painter.drawLine(this->width() - 26, (this->height() - 30.0) - (i * step) , this->width() - 30, (this->height() - 30.0) - (i * step));
		painter.drawText(this->width() - 24, (this->height() - 30.0) - (i * step) + 4, QString::number(m_ticks2[i]));
	}
	// la griglia la disegna  drawAxis1
}

void TraceWidget::drawFifo(int index, QPainter &painter){
	//PVNLAB_DEBUG;
	if(m_trace_scale[index] > 0){
		QPen pen(Qt::SolidLine);
		if(m_trace_scale[index] == 2){
			pen = QPen(Qt::DotLine);
		}
		pen.setColor(m_colors[index]);
		pen.setWidthF(1);
		painter.setPen(pen);
		float x_step = (this->width() - 60.0) / (float)TRACE_FIFO_SIZE;
		int appo = 0;
		for(int i = (m_count - 1); i > 0; i--){
			if(m_trace_scale[index] == 1){
				painter.drawLine(x_step * appo + 30, scale1(m_fifo[index][i]) , x_step * (appo + 1) +30, scale1(m_fifo[index][i - 1]));
			}
			else if(m_trace_scale[index] == 2){
				painter.drawLine(x_step * appo + 30, scale2(m_fifo[index][i]) , x_step * (appo + 1) +30, scale2 (m_fifo[index][i - 1]));
			}
			appo++;
		}
	}
}

float TraceWidget::scale1(float val){
	//PVNLAB_DEBUG;
	float f = ((val - m_min1)*(this->height() - 30.0)/(m_max1 - m_min1));
	return (this->height() - 30.0 - f);
}

float TraceWidget::scale2(float val){
	//PVNLAB_DEBUG;
	float f = ((val - m_min2)*(this->height() - 30.0)/(m_max2 - m_min2));
	return (this->height() - 30.0 - f);
}

void TraceWidget::setTraceScale(int index, int scale){
	//PVNLAB_DEBUG;
	m_trace_scale[index] = scale;
}


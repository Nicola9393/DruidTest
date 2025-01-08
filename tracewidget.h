/****************************************************************************
**
** Copyright (C) 2013 Piovan S.p.a
**
**	Author: Enricomaria Pavan
**
**
****************************************************************************/

#ifndef TRACEWIDGET_H
#define TRACEWIDGET_H

#define TRACE_COUNT 6
#define TRACE_FIFO_SIZE 100

#include <QWidget>

QT_BEGIN_NAMESPACE

namespace Ui {
	class TraceWidget;
}

QT_END_NAMESPACE

class TraceWidget : public QWidget
{
	Q_OBJECT

public:
	explicit TraceWidget(QWidget *parent = 0);
	~TraceWidget();

	void push(float v0, float v1, float v2, float v3, float v4, float v5);
	void setScale1(float min, float max);
	void setScale2(float min, float max);
	void setTraceScale(int index, int scale1);

public slots:
	void clear();

protected:
	void paintEvent(QPaintEvent* event);

private:
	void drawXAxis(QPainter &painter);
	void drawYAxis1(QPainter &painter);
	void drawYAxis2(QPainter &painter);
	float scale1(float val);
	float scale2(float val);

	void drawFifo(int index, QPainter &painter);

	Ui::TraceWidget *ui;

	int m_ticks1[11], m_ticks2[11];

	float m_fifo[TRACE_COUNT][TRACE_FIFO_SIZE];
	int m_trace_scale[TRACE_COUNT];
	int m_count;
	float m_min1, m_min2;
	float m_max1, m_max2;
	QColor m_colors[TRACE_COUNT];



};

#endif // TRACEWIDGET_H

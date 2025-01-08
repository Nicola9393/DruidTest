#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

class Logger : public QObject
{
	Q_OBJECT

public:
	static bool log(QString log, QString line);


signals:

public slots:

};

#endif // LOGGER_H

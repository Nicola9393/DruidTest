#include "logger.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

bool Logger::log(QString log, QString line){
	bool done = false;
	QFile file(log);
	if (file.open(QIODevice::Append | QIODevice::Text)){
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out<<line<<"\n";
		file.close();
		done = true;
	}
	else{
		qDebug()<<file.errorString()<<log;
		done = false;
	}
	return done;
}

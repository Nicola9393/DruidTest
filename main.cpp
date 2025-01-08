/***************************************************************************
**                                                                        **
** Copyright (C)  2015 by Piovan S.p.a                                    **
**                                                                        **
**	Author: Enricomaria Pavan                                         **
**                                                                        **
**                                                                        **
****************************************************************************/

#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#include <QMessageBox>
#include "pvnlab_app_lock.h"
#include <QLoggingCategory>

int main(int argc, char *argv[])
{
	//QLoggingCategory::setFilterRules(QStringLiteral("qt.modbus* = true"));
	QApplication app(argc, argv);

	// se il sistema non è in italiano allora uso sempre inglese.
	QLocale defloc(QLocale::Italian, QLocale::Italy);
	if(not QLocale::system().name().startsWith("it_")){
		defloc = QLocale(QLocale::English, QLocale::UnitedStates);
	}

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + defloc.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator myappTranslator;
	myappTranslator.load("shaman_" + defloc.name(), qApp->applicationDirPath());
	app.installTranslator(&myappTranslator);


	app.setOrganizationName("Piovan S.p.a");
	app.setApplicationName("DruidLAB");
	app.setApplicationVersion("0.2.1");
	Q_INIT_RESOURCE(resources);

	// *** build enviroment

	if(!QDir::home().exists(".piovan")){
		QDir::home().mkdir(".piovan");
	}
	if(!QDir::home().exists("/.piovan/druid_lab")){
		QDir::home().mkdir(".piovan/druid_lab");
	}
	if(!QDir::home().exists("/.piovan/druid_lab/json_maps")){
		QDir::home().mkdir(".piovan/druid_lab/json_maps");
	}

	if(PvnlabAppLock::authorizeApp(QDir::homePath() + "/.piovan/druid_lab/unlock.druid_lab")){
		MainWindow w;
		w.show();
		return app.exec();
	}
	else{
		QMessageBox::critical(0, app.applicationName() + QObject::tr(" - BLOCCATO"), QObject::tr("Applicazione non autorizzata!"));
		return -1;
	}
}

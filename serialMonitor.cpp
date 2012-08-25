#include <QMetaType>
#include <iostream>
#include <QVector>
#include <QMessageBox>
#include <QString>
#include <boost/asio.hpp>
#include <boost/exception/all.hpp>
#include "serialMonitor.h"
#include "ui_serialMonitor.h"
#include "serial.h"
#include "frame.h"

SerialMonitor::SerialMonitor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::SerialMonitor),
	subWindows(3),
	axesNames(3)
{
	ui->setupUi(this);

	//Inicializamos los nombres de los ejes de cada curva
	axesNames[0] = (tr("x axis"));
	axesNames[1] = (tr("y axis"));
	axesNames[2] = (tr("z axis"));

	//Creamos las sub-ventanas del mdi
	subWindows[0] = new QMdiSubWindow;
	subWindows[1] = new QMdiSubWindow;
	subWindows[2] = new QMdiSubWindow;
	//Eliminamos el botón de cerrar de las sub-ventanas
	//TODO:Si se pueden cerrar hacer que se puedan abrir de nuevo
	//subWindows[0]->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint);
	//subWindows[1]->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint);
	//subWindows[2]->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint);

	//Creamos las gráficas
	subWindows[0]->setWidget(new Plotter(tr("Gyroscope"), axesNames,this));
	subWindows[1]->setWidget(new Plotter(tr("Accelerometer"), axesNames,this));
	subWindows[2]->setWidget(new Plotter(tr("Magnetometer"), axesNames,this));

	//Añadimos las gráficas al mdi
	for(int i = 0 ; i < subWindows.size() ; ++i){
		ui->mdiArea->addSubWindow(subWindows[i]);
		subWindows[i]->show();
	}

	//Conectamos las señales del hilo de Serial
	connect(&serial, SIGNAL(started()), this, SLOT(connectedSerial()));
	connect(&serial, SIGNAL(finished()), this, SLOT(disconnectedSerial()));
	connect(&serial, SIGNAL(terminated()), this, SLOT(disconnectedSerial()));
	//Conectamos la señal de nueva trama para recoger las tramas
	qRegisterMetaType<Frame>("Frame");
	connect(&serial, SIGNAL(newFrame(Frame)), this, SLOT(newFrame(Frame)));
}

SerialMonitor::~SerialMonitor(){
	delete ui;
}

void SerialMonitor::connectSerial(){
	//Conectando
	ui->connectButton->setDisabled(true);
	ui->connectButton->setText(tr("Connecting..."));

	//Intentamos conectar
	try{
		serial.connect();
	}
	catch(std::exception &e){
		QMessageBox(QMessageBox::Warning,
				tr("unable to connect to port %1").arg(serialPort),
				e.what(),
				QMessageBox::Ok
			   ).exec();
	}
	catch(...){
		QMessageBox(QMessageBox::Warning,
				tr("unable to connect to port %1").arg(serialPort),
				tr("unknown error"),
				QMessageBox::Ok
			   ).exec();
	}

	if(serial.port.is_open()){
		serial.start();
		
	}
	else{
		//Desconectado
		ui->connectButton->setEnabled(true);
		ui->connectButton->setText(tr("Connect"));
	}
}

void SerialMonitor::disconnectSerial(){
	serial.disconnect();
}

void SerialMonitor::setSerialPort(const QString &sPort){
	serialPort = sPort;
}

void SerialMonitor::setBaudRate(int bRate){
	baudRate = bRate;
}

void SerialMonitor::newFrame(Frame frame){
	QVector<double> ys(3);

	ys[0] = frame.gx;
	ys[1] = frame.gy;
	ys[2] = frame.gz;
	((Plotter*)(subWindows[0]->widget()))->newData(frame.time, ys);

	ys[0] = frame.ax;
	ys[1] = frame.ay;
	ys[2] = frame.az;
	((Plotter*)(subWindows[1]->widget()))->newData(frame.time, ys);

	ys[0] = frame.mx;
	ys[1] = frame.my;
	ys[2] = frame.mz;
	((Plotter*)(subWindows[2]->widget()))->newData(frame.time, ys);
}

void SerialMonitor::connectedSerial(){
	//Conectado
	disconnect(ui->connectButton, SIGNAL(clicked()), this, SLOT(connectSerial()));
	connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(disconnectSerial()));

	ui->connectButton->setEnabled(true);
	ui->connectButton->setText(tr("Disconnect"));
}

void SerialMonitor::disconnectedSerial(){
	//Desconectado
	connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(connectSerial()));
	disconnect(ui->connectButton, SIGNAL(clicked()), this, SLOT(disconnectSerial()));

	ui->connectButton->setEnabled(true);
	ui->connectButton->setText(tr("Connect"));
}

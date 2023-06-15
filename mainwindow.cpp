#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "QtCore/qdebug.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->sliderX, SIGNAL(valueChanged(int)), ui->lcdX, SLOT(display(int)));
    QObject::connect(ui->sliderY, SIGNAL(valueChanged(int)), ui->lcdY, SLOT(display(int)));
    QObject::connect(ui->sliderZ, SIGNAL(valueChanged(int)), ui->lcdZ, SLOT(display(int)));
    QObject::connect(ui->maxX, SIGNAL(valueChanged(int)), this, SLOT(setSliderXMax(int)));
    QObject::connect(ui->maxY, SIGNAL(valueChanged(int)), this, SLOT(setSliderYMax(int)));
    QObject::connect(ui->maxZ, SIGNAL(valueChanged(int)), this, SLOT(setSliderZMax(int)));
    statusBar()->showMessage(tr("Welcome! set broker data and presso Connect, then topic and press Subscribe"), 10000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setSliderXMax(int max) {
    ui->sliderX->setMaximum(max);
}

void MainWindow::setSliderYMax(int max) {
    ui->sliderY->setMaximum(max);
}

void MainWindow::setSliderZMax(int max) {
    ui->sliderZ->setMaximum(max);
}

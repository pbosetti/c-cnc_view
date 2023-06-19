#include "mainwindow.h"
#include "./ui_mainwindow.h"
// #include "QtCore/qdebug.h"
#include <QtWidgets/QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>
#include <algorithm>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->subscribeButton->setEnabled(false);
    this->window()->setWindowTitle("C-CNC realtime data viewer");

    // Setup sliders
    QObject::connect(ui->sliderX, SIGNAL(valueChanged(int)), ui->lcdX, SLOT(display(int)));
    QObject::connect(ui->sliderY, SIGNAL(valueChanged(int)), ui->lcdY, SLOT(display(int)));
    QObject::connect(ui->sliderZ, SIGNAL(valueChanged(int)), ui->lcdZ, SLOT(display(int)));
    QObject::connect(ui->maxX, SIGNAL(valueChanged(int)), this, SLOT(setSliderXMax(int)));
    QObject::connect(ui->maxY, SIGNAL(valueChanged(int)), this, SLOT(setSliderYMax(int)));
    QObject::connect(ui->maxZ, SIGNAL(valueChanged(int)), this, SLOT(setSliderZMax(int)));
    statusBar()->showMessage(QString("Welcome! set broker data and press Connect, then topic and press Subscribe"), 10000);

    // Setup MQTT
    m_client = new QMqttClient(this);
    m_client->setHostname(ui->brokerAddressField->text());
    m_client->setPort(static_cast<quint16>(ui->brokerPortField->value()));
    connect(m_client, SIGNAL(disconnected()), this, SLOT(brokerDisconnected()));
    connect(m_client, &QMqttClient::messageReceived, this, &MainWindow::dealWithMessage);
    connect(ui->brokerAddressField, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
    connect(ui->brokerPortField, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setClientPort);

    // Setup tracePlot
    QPen pen;
    pen.setColor(QColor(200, 0, 0));
    pen.setStyle(Qt::CustomDashLine);
    QList<qreal> dashes;
    dashes << 20 << 2;
    pen.setDashPattern(dashes);
    // X
    ui->tracePlot->addGraph();
    ui->tracePlot->graph(0)->setPen(pen);

    // Y
    pen.setColor(QColor(0, 200, 0));
    ui->tracePlot->addGraph();
    ui->tracePlot->graph(1)->setPen(pen);

    // Z
    pen.setColor(QColor(0, 0, 200));
    ui->tracePlot->addGraph();
    ui->tracePlot->graph(2)->setPen(pen);

    pen.setStyle(Qt::SolidLine);
    // X
    pen.setColor(QColor(255, 0, 0));
    ui->tracePlot->addGraph();
    ui->tracePlot->graph(3)->setPen(pen);

    // Y
    pen.setColor(QColor(0, 255, 0));
    ui->tracePlot->addGraph();
    ui->tracePlot->graph(4)->setPen(pen);

    // Z
    pen.setColor(QColor(0, 0, 255));
    ui->tracePlot->addGraph();
    ui->tracePlot->graph(5)->setPen(pen);

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->tracePlot->xAxis->setTicker(timeTicker);
    ui->tracePlot->axisRect()->setupFullAxesBox();
    ui->tracePlot->yAxis->setRange(0, 1000);
    ui->tracePlot->setInteraction(QCP::iRangeDrag, true);
    ui->tracePlot->axisRect()->setRangeDrag(Qt::Horizontal);
    ui->tracePlot->setInteraction(QCP::iRangeZoom, true);
    ui->tracePlot->axisRect()->setRangeZoom(Qt::Horizontal);
    ui->tracePlot->xAxis->setLabel(QString("Elapsed time"));
    ui->tracePlot->yAxis->setLabel(QString("Axis Position (mm)"));

    connect(ui->maxX, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));
    connect(ui->maxY, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));
    connect(ui->maxZ, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));
    connect(ui->coordMinBox, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));
    connect(ui->coordMaxBox, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));

    // Setup XY plot
    xyCurveRapid = new QCPCurve(ui->xyPlot->xAxis, ui->xyPlot->yAxis);
    xyCurveInterp = new QCPCurve(ui->xyPlot->xAxis, ui->xyPlot->yAxis);
    xyCurveRapid->setPen(QPen(QColor(200, 0, 0)));
    xyCurveInterp->setPen(QPen(QColor(0, 0, 200)));
    xyCurvePosition = new QCPCurve(ui->xyPlot->xAxis, ui->xyPlot->yAxis);
    xyCurvePosition->setPen(QPen(QColor(0, 200, 0)));
    ui->xyPlot->axisRect()->setupFullAxesBox();
    ui->xyPlot->xAxis->setRange(0, ui->maxX->value());
    ui->xyPlot->yAxis->setRange(0, ui->maxY->value());
    ui->xyPlot->setInteraction(QCP::iRangeDrag, true);
    ui->xyPlot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    ui->xyPlot->setInteraction(QCP::iRangeZoom, true);
    ui->xyPlot->xAxis->setLabel(QString("X (mm)"));
    ui->xyPlot->yAxis->setLabel(QString("Y (mm)"));

    // Setup timers
    // Running timer
    timeCounter.start();
    // Timer for updating tracePlot
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        ui->tracePlot->replot();
        ui->xyPlot->replot();
    });
    timer->start(33);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_client;
}

void MainWindow::dealWithMessage(const QByteArray &message, const QMqttTopicName &topic) {
    double t = timeCounter.elapsed() / 1000.0;
    if (topic.name().endsWith(QString("setpoint"))) {
        QJsonDocument doc = QJsonDocument::fromJson(message);
        QJsonObject obj = doc.object();
        double x = obj["x"].toDouble();
        double y = obj["y"].toDouble();
        double z = obj["z"].toDouble();
        int rapid = obj["rapid"].toInt();
        ui->sliderX->setValue(x);
        ui->sliderY->setValue(y);
        ui->sliderZ->setValue(z);
        ui->tracePlot->graph(0)->addData(t, x);
        ui->tracePlot->graph(1)->addData(t, y);
        ui->tracePlot->graph(2)->addData(t, z);
        ui->tracePlot->xAxis->setRange(t, 60, Qt::AlignRight);
        (rapid ? xyCurveRapid : xyCurveInterp)->addData(x, y);
    } else if (topic.name().endsWith(QString("position"))) {
        QList<QByteArray> list = message.split(',');
        double x = list[0].toDouble();
        double y = list[1].toDouble();
        double z = list[2].toDouble();
        ui->tracePlot->graph(3)->addData(t, x);
        ui->tracePlot->graph(4)->addData(t, y);
        ui->tracePlot->graph(5)->addData(t, z);
        xyCurvePosition->addData(x, y);
    } else {
    }
}


void MainWindow::setPlotYRange() {
    std::vector<int> v {ui->maxX->value(), ui->maxY->value(), ui->maxZ->value()};
    int max = *std::max_element(v.begin(), v.end());
    ui->coordMinBox->setMaximum(max);
    ui->coordMaxBox->setMaximum(max);
    max = max < ui->coordMaxBox->value() ? max : ui->coordMaxBox->value();
    ui->tracePlot->yAxis->setRange(ui->coordMinBox->value(), max);
}

void MainWindow::setSliderXMax(int max) {
    ui->sliderX->setMaximum(max);
    ui->xyPlot->xAxis->setRange(0, ui->maxX->value());
}

void MainWindow::setSliderYMax(int max) {
    ui->sliderY->setMaximum(max);
    ui->xyPlot->yAxis->setRange(0, ui->maxY->value());
}

void MainWindow::setSliderZMax(int max) {
    ui->sliderZ->setMaximum(max);
}


void MainWindow::on_connectButton_clicked()
{
    bool disconnected = m_client->state() == QMqttClient::Disconnected;
    if (disconnected) {
        m_client->connectToHost();
    } else {
        m_client->disconnectFromHost();
    }
    ui->brokerAddressField->setEnabled(disconnected);
    ui->brokerPortField->setEnabled(disconnected);
    ui->subscribeButton->setEnabled(disconnected);
    ui->connectButton->setText(disconnected ? QString("Disconnect") : QString("Connect"));
    statusBar()->showMessage(disconnected ? QString("Connected") : QString("Disconnected"), 2000);
}


void MainWindow::brokerDisconnected()
{
    ui->brokerAddressField->setEnabled(true);
    ui->brokerPortField->setEnabled(true);
    ui->connectButton->setText(QString("Connect"));
    statusBar()->showMessage(QString("Unexpected disconnection"), 20000);
}

void MainWindow::setClientPort(int p)
{
    m_client->setPort(static_cast<quint16>(p));
}


void MainWindow::on_subscribeButton_clicked()
{
    auto subscription = m_client->subscribe(ui->topicField->text());
    if (!subscription) {
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not subscribe. Is there a valid connection?"));
        return;
    } else {
        statusBar()->showMessage(QString("Subscribed to topic ") + ui->topicField->text(), 2000);
    }
}



void MainWindow::on_clearDataButton_clicked()
{
    ui->tracePlot->graph(0)->data()->clear();
    ui->tracePlot->graph(1)->data()->clear();
    ui->tracePlot->graph(2)->data()->clear();
    xyCurveInterp->data().clear();
    xyCurveRapid->data().clear();
}


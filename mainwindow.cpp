#include "mainwindow.h"
#include "./ui_mainwindow.h"
// #include "QtCore/qdebug.h"
#include <QtWidgets/QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <algorithm>


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
    statusBar()->showMessage(tr("Welcome! set broker data and press Connect, then topic and press Subscribe"), 10000);
    m_client = new QMqttClient(this);
    m_client->setHostname(ui->brokerAddressField->text());
    m_client->setPort(static_cast<quint16>(ui->brokerPortField->value()));
    connect(m_client, SIGNAL(disconnected()), this, SLOT(brokerDisconnected()));

//    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
//        const QString content = QDateTime::currentDateTime().toString()
//                                + QLatin1String(" Received Topic: ")
//                                + topic.name()
//                                + QLatin1String(" Message: ")
//                                + message
//                                + QLatin1Char('\n');
//        ui->logTextArea->insertPlainText(content);
//    });
    connect(m_client, &QMqttClient::messageReceived, this, &MainWindow::dealWithMessage);
    connect(ui->brokerAddressField, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
    connect(ui->brokerPortField, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setClientPort);

    // X
    ui->plot->addGraph();
    ui->plot->graph(0)->setPen(QPen(QColor(255, 0, 0)));

    // Y
    ui->plot->addGraph();
    ui->plot->graph(1)->setPen(QPen(QColor(0, 255, 0)));

    // Z
    ui->plot->addGraph();
    ui->plot->graph(2)->setPen(QPen(QColor(0, 0, 255)));

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->plot->xAxis->setTicker(timeTicker);
    ui->plot->axisRect()->setupFullAxesBox();
    ui->plot->yAxis->setRange(0, 1000);

    connect(ui->maxX, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));
    connect(ui->maxY, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));
    connect(ui->maxZ, SIGNAL(valueChanged(int)), this, SLOT(setPlotYRange()));

    timer.start();
}

MainWindow::~MainWindow()
{
    delete m_client;
    delete ui;
}

void MainWindow::dealWithMessage(const QByteArray &message, const QMqttTopicName &topic) {
    const QString pattern = "\\/" + ui->streamComboBox->currentText() +"$";
    static QRegularExpression re(pattern);
    if (re.pattern() != pattern) {
        re.setPattern(pattern);
    }

    QRegularExpressionMatch match = re.match(topic.name());
    if (match.hasMatch()) {
        QJsonDocument doc = QJsonDocument::fromJson(message);
        QJsonObject obj = doc.object();
        QJsonValue x = obj["x"];
        QJsonValue y = obj["y"];
        QJsonValue z = obj["z"];
        QJsonValue rapid = obj["rapid"];
        ui->sliderX->setValue(x.toDouble());
        ui->sliderY->setValue(y.toDouble());
        ui->sliderZ->setValue(z.toDouble());
        double t = timer.elapsed() / 1000.0;
        ui->plot->graph(0)->addData(t, x.toDouble());
        ui->plot->graph(1)->addData(t, y.toDouble());
        ui->plot->graph(2)->addData(t, z.toDouble());
        ui->plot->xAxis->setRange(t, ui->timeSpanBox->value(), Qt::AlignRight);
        ui->plot->replot();
    } else {
        const QString content = QDateTime::currentDateTime().toString()
                                + QLatin1String(" Received Topic: ")
                                + topic.name()
                                + QLatin1String(" Message: ")
                                + message
                                + QLatin1Char('\n');
        ui->logTextArea->insertPlainText(content);
    }
    ui->logTextArea->ensureCursorVisible();
}


void MainWindow::setPlotYRange() {
    std::vector<int> v {ui->maxX->value(), ui->maxY->value(), ui->maxZ->value()};
    int max = *std::max_element(v.begin(), v.end());
    ui->plot->yAxis->setRange(0, max);
    ui->plot->replot();
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


void MainWindow::on_connectButton_clicked()
{
    if (m_client->state() == QMqttClient::Disconnected) {
        ui->brokerAddressField->setEnabled(false);
        ui->brokerPortField->setEnabled(false);
        ui->connectButton->setText(tr("Disconnect"));
        m_client->connectToHost();
        statusBar()->showMessage(tr("Connected"), 2000);
    } else {
        ui->brokerAddressField->setEnabled(true);
        ui->brokerPortField->setEnabled(true);
        ui->connectButton->setText(tr("Connect"));
        m_client->disconnectFromHost();
        statusBar()->showMessage(tr("Disconnected"), 2000);
    }
}


void MainWindow::brokerDisconnected()
{
    ui->brokerAddressField->setEnabled(true);
    ui->brokerPortField->setEnabled(true);
    ui->connectButton->setText(tr("Connect"));
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



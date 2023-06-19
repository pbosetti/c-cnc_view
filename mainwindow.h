#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qcustomplot.h"
#include <QMainWindow>
#include <QMqttClient>
#include <QElapsedTimer>



QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void setClientPort(int p);

private slots:
    void setSliderXMax(int max);
    void setSliderYMax(int max);
    void setSliderZMax(int max);
    void on_connectButton_clicked();
    void brokerDisconnected();
    void on_subscribeButton_clicked();
    void dealWithMessage(const QByteArray &message, const QMqttTopicName &topic);
    void setPlotYRange();
    void on_clearDataButton_clicked();

private:
    Ui::MainWindow *ui;
    QMqttClient *m_client;
    QCPCurve *xyCurveRapid;
    QCPCurve *xyCurveInterp;
    QCPCurve *xyCurvePosition;
    QElapsedTimer timeCounter;

};
#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMqttClient>



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

private:
    Ui::MainWindow *ui;
    QMqttClient *m_client;

};
#endif // MAINWINDOW_H

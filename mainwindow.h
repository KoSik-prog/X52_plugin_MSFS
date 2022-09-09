#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtXml>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_profil_comboBox_currentIndexChanged(const QString &arg1);

    void on_addProfile_pushButton_clicked();

    void on_removeProfil_Button_clicked();

    void on_save_Button_clicked();

    void on_Connect_Button_clicked();

    void readSimmconnectData();

    void on_Disconnect_Button_clicked();

    void x52_flash();

    void SetLed(Ui::MainWindow *ui_pointer);

    void mfdPrintLines();

    void autoconnect_timer();

    void on_autoconnect_checkBox_stateChanged(int arg1);

    void on_AboutPushButton_clicked();

private:
    Ui::MainWindow *ui;

    QTimer *TimerSimconnectReader; //Timer do odczytywania simconnect

    QTimer *TimerFlash; //Timer do blyskania diodami

    QTimer *TimerAutoconnect; //wiadomo

    QTimer *TimerMFDrefresh; //odswiezenie wyswietlacza

    void delay( int millisecondsToWait );
};
#endif // MAINWINDOW_H

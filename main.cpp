#include "mainwindow.h"

#include <QApplication>
#include <signal.h>
#include <QMessageBox>

void manageSegFailure(int signalCode)
{
    int userResult = QMessageBox::critical(nullptr, "Error", "Check Your Saitek X52PRO!", QMessageBox::Ok);

    if (userResult == QMessageBox::Yes) {
        // You can store the logging info here.
    }

    signal(signalCode, SIG_DFL);
    QApplication::exit(3);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    signal(SIGSEGV, manageSegFailure);
    MainWindow w;
    w.show();
    return a.exec();
}

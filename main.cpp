#include "mainwindow.h"

#include <QApplication>
#include <signal.h>
#include <QMessageBox>

#include <stdio.h>
#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

void manageSegFailure(int signalCode)
{
    char cCurrentPath[FILENAME_MAX];

     if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
         {
         //return errno;
         qDebug() <<  "path error";
         }

    cCurrentPath[sizeof(cCurrentPath) - 1] = '/0'; /* not really required */
    //int userResult = QMessageBox::critical(nullptr, "Error", QString::number(signalCode), QMessageBox::Ok);
    int userResult = QMessageBox::critical(nullptr, "Error", "Check Your Saitek X52PRO!", QMessageBox::Ok);

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

#ifndef XML_SETTINGS_H
#define XML_SETTINGS_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class xml_settings
{
public:
    xml_settings();
private:
    Ui::MainWindow *ui;
};

#endif // XML_SETTINGS_H

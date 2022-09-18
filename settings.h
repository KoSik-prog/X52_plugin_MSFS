#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <windows.h>
#include <qstring.h>
#include <QtXml>

class settings
{
public:
    void get_planes(QString tag, QString att);
    void get_functions(QString tag, QString arg1);
    void create_newProfile(QString name, QStringList settingsArray);
    void setAttribute(QDomDocument doc, QDomElement btnsFile, QString name, QString function, uint8_t type);
    void remove_profile(QString name);
};

#endif // SETTINGS_H

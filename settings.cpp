#include "settings.h"
#include <QtXml>
#include <QFile>
#include <QDebug>

extern QStringList profilesArray;
extern QStringList buttonsArray;
extern QStringList mfdArray;

void settings::get_planes(QString tag, QString att){
    QDomDocument document;
    QFile file("settings.xml");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open the file for reading.";
        }
        else
        {
            if(!document.setContent(&file))
            {
                qDebug() << "Failed to load the file for reading.";
            }
            file.close();
        }
        profilesArray.clear();
        //ui_pointer->profil_comboBox->clear();

    QDomElement root = document.firstChildElement();
    QDomNodeList nodes = root.elementsByTagName(tag);

    qDebug() << "PLANES = " << nodes.count();
    for(int i = 0; i < nodes.count(); i++)
    {
        QDomNode elm = nodes.at(i);
        if(elm.isElement())
        {
            QDomElement e = elm.toElement();
            profilesArray.append(e.attribute(att));
            //ui_pointer->profil_comboBox->addItem(e.attribute(att));
        }
    }
    profilesArray.sort();
    qDebug() << profilesArray;
}

void settings::get_functions(QString tag, QString name){
    QDomDocument document;
    QFile file("settings.xml");

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the file for reading.";
    }
    else
    {
        if(!document.setContent(&file))
        {
            qDebug() << "Failed to load the file for reading.";
        }
        file.close();
    }
    buttonsArray.clear();
    mfdArray.clear();
    QDomElement root = document.firstChildElement();
    QDomNodeList plane = root.elementsByTagName(tag);
    qDebug() << "reading settings for plane: " << name;
    for(int i = 0; i < plane.count(); i++)
    {
        QDomNode planenode = plane.at(i);
        if(planenode.isElement())
        {
            QDomElement planeelement = planenode.toElement();
            if(planeelement.attribute("name") == name){
                QDomNodeList nodes = planeelement.elementsByTagName("btn");
                for(int i = 0; i < nodes.count(); i++)
                {
                    QDomNode elm = nodes.at(i);
                    if(elm.isElement())
                    {
                        QDomElement e = elm.toElement();
                        qDebug() << "Btn name: " << e.attribute("name") << " function: " << e.attribute("function");
                        buttonsArray.append(e.attribute("function"));
                    }
                }
                QDomNodeList nodesMFD = planeelement.elementsByTagName("mfd");
                for(int i = 0; i < nodesMFD.count(); i++)
                {
                    QDomNode elemm = nodesMFD.at(i);
                    if(elemm.isElement())
                    {
                        QDomElement el = elemm.toElement();
                        qDebug() << "MFD name: " << el.attribute("name") << " function: " << el.attribute("function");
                        mfdArray.append(el.attribute("function"));
                    }
                }

            }
        }
    }
}

void settings::create_newProfile(QString name, QStringList settingsArray){
    QDomDocument doc;
    QFile file("settings.xml");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the file for reading.";
    }
    else
    {
        if(!doc.setContent(&file))
        {
            qDebug() << "Failed to load the file for reading.";
        }
        file.close();
    }
    QDomElement root = doc.firstChildElement();

    QDomElement btns = doc.createElement("plane");
    btns.setAttribute("name",name);
    root.appendChild(btns);
    //------------------------------------------------------------------
    setAttribute(doc, btns, "LED_FIRE", settingsArray.at(0), 0);
    setAttribute(doc, btns, "LED_FIRE_A", settingsArray.at(1), 0);
    setAttribute(doc, btns, "LED_FIRE_B", settingsArray.at(2), 0);
    setAttribute(doc, btns, "LED_FIRE_D", settingsArray.at(3), 0);
    setAttribute(doc, btns, "LED_FIRE_E", settingsArray.at(4), 0);
    setAttribute(doc, btns, "LED_TOGGLE_1_2", settingsArray.at(5), 0);
    setAttribute(doc, btns, "LED_TOGGLE_3_4", settingsArray.at(6), 0);
    setAttribute(doc, btns, "LED_TOGGLE_5_6", settingsArray.at(7), 0);
    setAttribute(doc, btns, "LED_POV_2", settingsArray.at(8), 0);
    setAttribute(doc, btns, "LED_CLUTCH", settingsArray.at(9), 0);
    setAttribute(doc, btns, "LED_THROTTLE", settingsArray.at(10), 0);
    setAttribute(doc, btns, "0", settingsArray.at(11), 1);;
    setAttribute(doc, btns, "1", settingsArray.at(12), 1);
    setAttribute(doc, btns, "2", settingsArray.at(13), 1);
    //------------------------------------------------------------------

    QFile file2("settings.xml");
    if(file2.open(QFile::WriteOnly | QFile::Text)){
        QTextStream in(&file2);
        in<<doc.toString();
        file2.flush();
        file2.close();
        qDebug()<<"finished.";
    }
    else qDebug()<<"file open failed.";
}


/*
 * type = 0 for LED, 1 for MFD
*/
void settings::setAttribute(QDomDocument doc, QDomElement btnsFile, QString name, QString function, uint8_t type){
    QDomElement btnElement;
    if(type == 0){
        btnElement = doc.createElement("btn");
    } else {
        btnElement = doc.createElement("mfd");
    }

    btnElement.setAttribute("name", name);
    btnElement.setAttribute("function", function);

    btnsFile.appendChild(btnElement);
}

void settings::remove_profile(QString name){
    QDomDocument doc;
    QFile file("settings.xml");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open the file for reading.";
        }
        else
        {
            if(!doc.setContent(&file))
            {
                qDebug() << "Failed to load the file for reading.";
            }
            file.close();
        }
        QDomElement root = doc.firstChildElement();
        QDomNodeList nodes = root.elementsByTagName("plane");

        qDebug() << "deleting plane = " << nodes.count();
        for(int i = 0; i < nodes.count(); i++)
        {
            QDomNode elm = nodes.at(i);
            if(elm.isElement())
            {
                QDomElement e = elm.toElement();
                if(e.attribute("name") == name){
                    root.removeChild(e);
                    qDebug() << "deleted";
                }
            }
        }

        if(file.open(QFile::WriteOnly | QFile::Text)){
            QTextStream in(&file);
            in<<doc.toString();
            file.flush();
            file.close();
        } else qDebug()<<"file open failed.";
}

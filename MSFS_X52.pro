QT       += core gui
QT       += xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    structures_simconnect.cpp \
    x52_output.cpp

HEADERS += \
    DirectOutput.h \
    leds.h \
    mainwindow.h \
    structures_simconnect.h \
    x52_output.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -lSimConnect
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -lSimConnect
else:unix: LIBS += -L$$PWD/./ -lSimConnect
win32: LIBS += -L$$PWD/./ -lDirectOutput
win32:RC_ICONS += x52_icon.ico

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.


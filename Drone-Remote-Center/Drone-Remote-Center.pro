#-------------------------------------------------
#
# Project created by QtCreator 2015-02-13T09:43:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Drone-Remote-Center
TEMPLATE = app

FORMS    = mainwindow.ui

SOURCES += main.cpp\
        mainwindow.cpp \
    communication.cpp

HEADERS  += mainwindow.h \
    communication.h

win32:LIBS += "$$PWD/libxbee3_v3.0.10/lib/libxbee3.lib"
else:linux: {
    LIBS += -L$$PWD/libxbee3_v3.0.10/lib/ -lxbee
    LIBS += -lboost_system -lboost_filesystem
}
INCLUDEPATH += $$PWD/libxbee3_v3.0.10/lib
DEPENDPATH += $$PWD/libxbee3_v3.0.10/lib

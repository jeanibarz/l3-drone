#-------------------------------------------------
#
# Project created by QtCreator 2015-02-13T09:43:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Drone-Remote-Center
TEMPLATE = app

win32:LIBS += "$$_PRO_FILE_PWD_/libxbee3_v3.0.10/lib/libxbee3.lib"
linux:LIBS += lxbee

#LIBS += -L"$$_PRO_FILE_PWD_/libxbee3_v3.0.10/lib" -lxbee

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

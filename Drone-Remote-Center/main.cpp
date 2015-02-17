#include "mainwindow.h"
#include <QApplication>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h> // for the sleep function under windows and linux

#if defined(WIN32)
#include <windows.h> // for the Sleep function under windows (take arg in milliseconds)
#endif

#include "libxbee3_v3.0.10/xbee.h"

#include "communication.h"

#define XBEE_TEST 0 // Switch to 1 to activate XBEE test code

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

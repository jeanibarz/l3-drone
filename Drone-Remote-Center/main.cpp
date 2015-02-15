#include "mainwindow.h"
#include <QApplication>

#include <stdio.h>
#include <stdlib.h>

/* THIS SHOULD BE INCLUDE UNDER WINDOWS ONLY */
#include <unistd.h> // for the sleep function under windows (take arg in seconds)
#include <windows.h> // for the Sleep function under windows (take arg in milliseconds)
/* THIS SHOULD BE INCLUDE UNDER WINDOWS ONLY */

#include "libxbee3_v3.0.10/xbee.h"

void myCB(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {
        if ((*pkt)->dataLen == 0) {
                printf("too short...\n");
                return;
        }
        printf("rx: [%s]\n", (*pkt)->data);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    void *d;
    struct xbee *xbee;
    struct xbee_con *con;
    unsigned char txRet;
    xbee_err ret;

    /* windows: if this doesn't work, try something like the following:
            xbee_setup(&xbee, "xbee1", "\\\\.\\COM25", 57600);

       linux: use "/dev/ttyUSB0" or something like that instead of "COM8"
    */
    if ((ret = xbee_setup(&xbee, "xbee1", "COM8", 57600)) != XBEE_ENONE) {
            printf("ret: %d (%s)\n", ret, xbee_errorToStr(ret));
            return ret;
    }

    if ((ret = xbee_conNew(xbee, &con, "Local AT", NULL)) != XBEE_ENONE) {
            xbee_log(xbee, -1, "xbee_conNew() returned: %d (%s)", ret, xbee_errorToStr(ret));
            return ret;
    }

    if ((ret = xbee_conCallbackSet(con, myCB, NULL)) != XBEE_ENONE) {
            xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", ret);
            return ret;
    }

    ret = xbee_conTx(con, &txRet, "NI");
    printf("tx: %d\n", ret);
    if (ret) {
            printf("txRet: %d\n", txRet);
    } else {
            Sleep(1000);
    }

    if ((ret = xbee_conEnd(con)) != XBEE_ENONE) {
            xbee_log(xbee, -1, "xbee_conEnd() returned: %d", ret);
            return ret;
    }

    xbee_shutdown(xbee);

    return a.exec();
}

#include "mainwindow.h"
#include <QApplication>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32)
#include <unistd.h> // for the sleep function under windows (take arg in seconds)
#include <windows.h> // for the Sleep function under windows (take arg in milliseconds)
#endif

#include "libxbee3_v3.0.10/xbee.h"

#include "communication.h"

void processLoop(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {
        if ((*pkt)->dataLen == 0) {
                printf("too short...\n");
                return;
        }
        printf("rx: [%s]\n", (*pkt)->data);

        if((*pkt)->dataLen == sizeof(rxPacket)) {
            struct rxPacket * rx_data = new rxPacket((*pkt)->data);
            printf("\t\tpacket_clock:%d\n", rx_data->packet_clock_);
            printf("\t\tpower_charge:%d\n", rx_data->power_charge_);
            printf("\t\tgyro:(%d, %d, %d)\n", rx_data->gyro_.roll_, rx_data->gyro_.yaw_, rx_data->gyro_.pitch_);
            printf("\t\taccel:(%d, %d, %d)\n", rx_data->accel_.longitudinal_, rx_data->accel_.lateral_, rx_data->accel_.vertical_);
            delete rx_data;
        }

        struct txPacket * tx_pkt = new txPacket(0,0,0,0);
        unsigned char ret_val;
        xbee_sendTxPacket(con, &ret_val, tx_pkt);
}

#define XBEE_TEST 0 // Switch to 1 to activate XBEE test code

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    if (XBEE_TEST)
    {
        void *d;
        struct xbee *xbee;
        struct xbee_con *con;
        struct xbee_conAddress drone_address;
        unsigned char txRet;
        xbee_err ret;

        /* windows: if this doesn't work, try something like the following:
                xbee_setup(&xbee, "xbee1", "\\\\.\\COM25", 57600);

           linux: use "/dev/ttyUSB0" or something like that instead of "COM8"
        */

        // Configure l'utilisation du modele de librairie "xbee1", port usb "COM8", transmission 57600 b/s
        if ((ret = xbee_setup(&xbee, "xbee1", "COM8", 57600)) != XBEE_ENONE) {
                printf("ret: %d (%s)\n", ret, xbee_errorToStr(ret));
                return ret;
        }

        // Configuration de l'addresse 64 bits du drone
        memset(&drone_address, 0, sizeof(drone_address));
        drone_address.addr64_enabled = 1;
        drone_address.addr64[0] = 0x00;
        drone_address.addr64[1] = 0x13;
        drone_address.addr64[2] = 0xA2;
        drone_address.addr64[3] = 0x00;
        drone_address.addr64[4] = 0x40;
        drone_address.addr64[5] = 0x08;
        drone_address.addr64[6] = 0x18;
        drone_address.addr64[7] = 0x26;


        // Configure une nouvelle connection sur la xbee, de type "64-bit Data" (bidirectionnelle) avec l'addresse du drone pour destination
        if ((ret = xbee_conNew(xbee, &con, "64-bit Data", &drone_address)) != XBEE_ENONE) {
                xbee_log(xbee, -1, "xbee_conNew() returned: %d (%s)", ret, xbee_errorToStr(ret));
                return ret;
        }

        /*
        // Configure un callback sur la fonction "myCB" sur la connection "con": la fonction "myCB" sera appellée a chaque paquet recu
        if ((ret = xbee_conCallbackSet(con, processLoop, NULL)) != XBEE_ENONE) {
                xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", ret);
                return ret;
        }*/

        // xbee_err xbee_connTx(struct xbee_con *con, unsigned char *retVal, const unsigned char *buf, int len);

        ret = xbee_conTx(con, &txRet, "NI");
        printf("tx: %d\n", ret);
        if (ret) {
                printf("txRet: %d\n", txRet);
        } else {
                Sleep(1000);
        }

        // Ferme la connection "con"
        if ((ret = xbee_conEnd(con)) != XBEE_ENONE) {
                xbee_log(xbee, -1, "xbee_conEnd() returned: %d", ret);
                return ret;
        }

        // Terminate all remaining connections and free all data associated with the instance of libxbee.
        xbee_shutdown(xbee);
    }

    return a.exec();
}

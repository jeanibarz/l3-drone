#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "libxbee3_v3.0.10/xbee.h"
#include "communication.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&timer, SIGNAL(timeout()), this, SLOT(doWork()));

    struct xbee *xbee;
    struct xbee_con *con;
    struct xbee_conAddress drone_address;
    xbee_err ret;

    /* windows: if this doesn't work, try something like the following:
            xbee_setup(&xbee, "xbee1", "\\\\.\\COM25", 57600);

       linux: use "/dev/ttyUSB0" or something like that instead of "COM8"
    */

    // Configure l'utilisation du modele de librairie "xbee1", port usb "COM8", transmission 57600 b/s
    if ((ret = xbee_setup(&xbee, "xbee1", "COM8", 57600)) != XBEE_ENONE) {
            printf("ret: %d (%s)\n", ret, xbee_errorToStr(ret));
            // return ret;
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
            // return ret;
    }

    counter = 0;
    timer.start(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}

xbee_err MainWindow::doWork()
{
    ++counter;
    if(counter%10==0) counter = 0;
    ui->dial->setValue(counter);

    // process each packet until Xbee buffer is empty
    int remainingPackets;
    struct xbee_pkt *pkt;
    xbee_err ret;

    do
    {
          if ((ret = xbee_conRx(con, &pkt, &remainingPackets)) != XBEE_ENONE)
          {
              xbee_log(xbee, -1, "xbee_conRx() returned: %d (%s)", ret, xbee_errorToStr(ret));
              return ret;
          }

          if(pkt->dataLen == sizeof(rxPacket)) {
              struct rxPacket * rx_data = new rxPacket(pkt->data);
              printf("\t\tpacket_clock:%d\n", rx_data->packet_clock_);
              printf("\t\tpower_charge:%d\n", rx_data->power_charge_);
              printf("\t\tgyro:(%d, %d, %d)\n", rx_data->gyro_.roll_, rx_data->gyro_.yaw_, rx_data->gyro_.pitch_);
              printf("\t\taccel:(%d, %d, %d)\n", rx_data->accel_.longitudinal_, rx_data->accel_.lateral_, rx_data->accel_.vertical_);
              delete rx_data;

              if ((ret = xbee_pktFree(pkt)) != XBEE_ENONE) return ret;
          }
    } while (remainingPackets > 0);
    // process refreshing window
    return XBEE_ENONE;
}

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QCloseEvent>

#include "libxbee3_v3.0.10/xbee.h"
#include "communication.h"

#define XBEE_TEST 0

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    rx_packets_counter(0), tx_packets_counter(0),
    latency_buffer_length(20), latency_index(0),
    current_state(STARTING)
{
    calloc(latency_buffer_length,sizeof(latency_buffer[0]));

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

    ui->setupUi(this);
    connect(&timer, SIGNAL(timeout()), this, SLOT(doWork()));

    counter = 0;
    rx_packets_counter = 0;
    tx_packets_counter = 0;
    timer.start(50);
    log_timer.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

xbee_err MainWindow::doWork()
{
    switch(current_state)
    {
        case STARTING:
            /* windows: if this doesn't work, try something like the following:
                    xbee_setup(&xbee, "xbee1", "\\\\.\\COM25", 57600);

               linux: use "/dev/ttyUSB0" or something like that instead of "COM8"
            */

            //if(!XBEE_TEST) break;

            // Configure l'utilisation du modele de librairie "xbee1", port usb "COM8", transmission 57600 b/s
            if ((ret = xbee_setup(&xbee, "xbee1", "COM8", 57600)) != XBEE_ENONE) {
                printf("ret: %d (%s)\n", ret, xbee_errorToStr(ret));
                current_state = EMERGENCY;
                break;
            }
            else {
                current_state = CONNECTING;
            }
            break;
        case CONNECTING:
            // Configure une nouvelle connection sur la xbee, de type "64-bit Data" (bidirectionnelle) avec l'addresse du drone pour destination
            if ((ret = xbee_conNew(xbee, &con, "64-bit Data", &drone_address)) != XBEE_ENONE) {
                if(xbee) xbee_log(xbee, -1, "xbee_conNew() returned: %d (%s)", ret, xbee_errorToStr(ret));
                break;
            }
            else {
                current_state = WORKING;
            }
            break;
        case WORKING:
            /* FOR FUN */
            ++counter;
            if(counter%100==0) counter = 0;


            struct xbee_conInfo conInfo;
            if ((ret = xbee_conInfoGet(con, &conInfo)) != XBEE_ENONE) {
                if(xbee) xbee_log(xbee, -1, "xbee_conInfoGet() returned: %d (%s)", ret, xbee_errorToStr(ret));
                break;
            }

            ui->spinBox_tx_packets->setValue(conInfo.countTx);
            ui->spinBox_rx_packets->setValue(conInfo.countRx);
            break;
        case DISCONNECTING:
            if ((ret = xbee_conEnd(con)) != XBEE_ENONE) {
                if(xbee) xbee_log(xbee, -1, "xbee_conEnd() returned: %d (%s)", ret, xbee_errorToStr(ret));
                break;
            }
            break;
        case EMERGENCY:
            break;
    };


    ui->progressBar->setValue(counter);

    // process each packet until Xbee buffer is empty
    int remainingPackets;
    struct xbee_pkt *pkt = NULL;
/*
    do
    {
          if ((ret = xbee_conRx(con, &pkt, &remainingPackets)) != XBEE_ENONE)
          {
              xbee_log(xbee, -1, "xbee_conRx() returned: %d (%s)", ret, xbee_errorToStr(ret));
              return ret;
          }

          if(pkt == NULL) break;

          ++rx_packets_counter;
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
*/
    latency_buffer[latency_index] = tx_packets_counter%40;
    latency_index = (latency_index+1)%latency_buffer_length;

    ++tx_packets_counter;

    ui->spinBox_avg_latency->setValue(calculateAvgLatency());
    ui->spinBox_tx_packets->setValue(tx_packets_counter);
    ui->spinBox_rx_packets->setValue(rx_packets_counter);
    ui->timeEdit_log_time->setTime(QTime(0,0,0,log_timer.elapsed()));

    return XBEE_ENONE;
}

uint16_t MainWindow::calculateAvgLatency()
{
    uint8_t i;
    uint16_t average = 0;

    for(i = 0; i < latency_buffer_length; ++i)
    {
        average += latency_buffer[i];
    }
    average /= latency_buffer_length;

    return average;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Fermeture",
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        if(xbee) {
            current_state = DISCONNECTING;
            doWork();
            if ((ret = xbee_shutdown(xbee)) != XBEE_ENONE) {
                if(xbee) xbee_log(xbee, -1, "xbee_shutdown() returned: %d (%s)", ret, xbee_errorToStr(ret));
            }
        }
        event->accept();
    }
}

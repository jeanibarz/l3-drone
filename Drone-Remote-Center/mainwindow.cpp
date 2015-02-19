#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QCloseEvent>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "libxbee3_v3.0.10/xbee.h"
#include "communication.h"

#define XBEE_TEST 0

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    rx_packets_counter(0), tx_packets_counter(0),
    latency_buffer_length(20), latency_index(0),
    current_state(STARTING), error_msg(QString("")), logging(false)
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
    statusBar()->addWidget(&statusBar_current_state);

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

void MainWindow::doWork()
{
    switch(current_state)
    {
        case STARTING:
        { // We need these brackets because local variables shouldn't jump from case to case
            statusBar_current_state.setText(QString("STARTING"));

            /* windows: if this doesn't work, try something like the following:
                    xbee_setup(&xbee, "xbee1", "\\\\.\\COM25", 57600);

               linux: use "/dev/ttyUSB0" or something like that instead of "COM8"
            */

            //if(!XBEE_TEST) break;

            // Configure l'utilisation du modele de librairie "xbee1", port usb "COM8", transmission 57600 b/s
            if ((ret = xbee_setup(&xbee, "xbee1", "COM8", 57600)) != XBEE_ENONE) {
                error_msg = QString(boost::str(boost::format("xbee_setup() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                current_state = EMERGENCY;
                return;
            }
            else {
                current_state = CONNECTING;
            }
            break;
        }
        case CONNECTING:
        {
            statusBar_current_state.setText(QString("CONNECTING"));

            // Configure une nouvelle connection sur la xbee, de type "64-bit Data" (bidirectionnelle) avec l'addresse du drone pour destination
            if ((ret = xbee_conNew(xbee, &con, "64-bit Data", &drone_address)) != XBEE_ENONE) {
                error_msg = QString(boost::str(boost::format("xbee_conNew() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                current_state = EMERGENCY;
                return;
            }
            else {
                current_state = WORKING;
            }
            break;
        }
        case WORKING:
        {
            statusBar_current_state.setText(QString("WORKING"));

            { // FUNNY PROGRESS BAR UPDATE
                ++counter;
                if(counter%100==0) counter = 0;
                ui->progressBar->setValue(counter);
            }
            { // UPDATE CONNECTION INFORMATIONS (RX/TX PACKETS)
                struct xbee_conInfo conInfo;
                if ((ret = xbee_conInfoGet(con, &conInfo)) != XBEE_ENONE) {
                    error_msg = QString(boost::str(boost::format("xbee_conInfoGet() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                    current_state = EMERGENCY;
                    return;
                }
                ui->spinBox_tx_packets->setValue(conInfo.countTx);
                ui->spinBox_rx_packets->setValue(conInfo.countRx);
            }
            { // PROCESSING RECEIVED PACKETS
                // process each packet until Xbee buffer is empty
                int remainingPackets;
                struct xbee_pkt *pkt = NULL;

                do // loop until received packets buffer is empty
                {
                      if ((ret = xbee_conRx(con, &pkt, &remainingPackets)) != XBEE_ENONE) {
                          error_msg = QString(boost::str(boost::format("xbee_conRx() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                          current_state = EMERGENCY;
                          return;
                      }

                      if(pkt == NULL) break; // just in case...

                      ++rx_packets_counter;
                      if(pkt->dataLen == sizeof(rxPacket)) {
                          struct rxPacket * rx_data = new rxPacket(pkt->data);
                          rx_data->print();
                          delete rx_data;

                          if ((ret = xbee_pktFree(pkt)) != XBEE_ENONE) {
                              error_msg = QString(boost::str(boost::format("xbee_pktFree() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                              current_state = EMERGENCY;
                              return;
                          }
                      }
                } while (remainingPackets > 0);
            }
            {
                // process refreshing window
                latency_buffer[latency_index] = tx_packets_counter%40; // to be minded (how to ping ?)
                latency_index = (latency_index+1)%latency_buffer_length; // to be minded (how to ping ?)

                ++tx_packets_counter; // to be removed when testing Xbee

                ui->spinBox_avg_latency->setValue(calculateAvgLatency());
                ui->spinBox_tx_packets->setValue(tx_packets_counter); // to be removed when testing Xbee
                ui->spinBox_rx_packets->setValue(rx_packets_counter); // to be removed when testing Xbee
                ui->timeEdit_log_time->setTime(QTime(0,0,0,log_timer.elapsed())); // why this doesn't work ?

            }
            break;
        }
        case DISCONNECTING:
        {
            statusBar_current_state.setText(QString("DISCONNECTING"));
            if ((ret = xbee_conEnd(con)) != XBEE_ENONE) {
                error_msg = QString(boost::str(boost::format("xbee_conEnd() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                current_state = EMERGENCY;
                return;
            }
            break;
        }
        case EMERGENCY: // we should implement a logging of xbee_errors ?
        {
            QString text("EMERGENCY");
            text.append(" - ");
            text.append(error_msg);

            statusBar_current_state.setText(text);

            // FOR TEST
            if (logging) {
                fprintf(log_filestream, "EMERGENCY\n");
                fflush(log_filestream);
                ui->timeEdit_log_time->setTime(QTime(0,0,0,0).addMSecs(log_timer.elapsed()));
                ui->doubleSpinBox_log_size->setValue(boost::filesystem::file_size(log_filepath)/1024.0); // 1 kb = 1024 bytes
            }
            break;
        }
        default: // we're not supposed to be here
        {
            error_msg = QString("Unexpected state");
            current_state = EMERGENCY;
            return;
            break;
        }
    };
}

uint16_t MainWindow::calculateAvgLatency()
{
    u_int8_t i;
    u_int16_t average = 0;

    for(i = 0; i < latency_buffer_length; ++i)
    {
        average += latency_buffer[i];
    }
    average /= latency_buffer_length;

    return average;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Exiting",
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::No | QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        if(xbee) {
            current_state = DISCONNECTING;
            doWork();
            if ((ret = xbee_shutdown(xbee)) != XBEE_ENONE) {
                error_msg = QString(boost::str(boost::format("xbee_shutdown() error (%d - %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                // we don't care except if we need to log the error in a file
            }
        }
        if(logging && log_filestream != NULL) {
            fclose(log_filestream);
        }
        event->accept();
    }
}

void MainWindow::on_pushButton_start_logging_clicked()
{
    logging = true;
    boost::filesystem::path log_dir_path("./" + ui->lineEdit_log_dir_path->text().toStdString());
    if(!boost::filesystem::exists(log_dir_path))
    {
        if (!boost::filesystem::create_directory(log_dir_path)) {
            error_msg = "create_directory() error";
            current_state = EMERGENCY;
            return;
        }
    }

    uint8_t log_number = 1;
    std::string log_filename;
    while(true) {
        log_filename = log_dir_path.string() + "/log" + boost::lexical_cast<std::string>((int)log_number) + ".txt";
        if(!boost::filesystem::exists(boost::filesystem::path(log_filename))
        ) {
            break;
        }
        ++log_number;

        if(log_number == 255) {
            error_msg = "log_filename infinite loop while ?";
            current_state = EMERGENCY;
            return;
        }
    }

    log_filestream = fopen(log_filename.c_str(), "w");
    if (log_filestream == NULL) {
        error_msg = "fopen() error (invalid handle)";
        current_state = EMERGENCY;
        return;
    }
    else
    {
        logging = true;
        std::string log_state_text = "Logging to " + log_filename;
        log_filepath = boost::filesystem::path(log_filename);
        ui->lineEdit_log_state->setText(log_state_text.c_str());
        ui->pushButton_start_logging->setText("START NEW");
        ui->pushButton_stop_logging->setEnabled(true);
        ui->doubleSpinBox_log_size->setEnabled(true);
        log_timer = QTime(0,0,0,0);
        log_timer.start();
        ui->timeEdit_log_time->setTime(log_timer);
        ui->timeEdit_log_time->setEnabled(true);
        return;
    }
}

void MainWindow::on_pushButton_stop_logging_clicked()
{
    if(logging) {
        logging = false;
        fclose(log_filestream);
        ui->lineEdit_log_state->setText("Idle");
        ui->pushButton_start_logging->setText("START");
        ui->pushButton_stop_logging->setEnabled(false);
        ui->doubleSpinBox_log_size->setValue(0);
        ui->doubleSpinBox_log_size->setEnabled(false);
        ui->timeEdit_log_time->setTime(QTime(0,0,0,0));
        ui->timeEdit_log_time->setEnabled(false);
    }
}

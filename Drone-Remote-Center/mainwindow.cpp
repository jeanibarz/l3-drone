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
    current_state(STARTING), last_state(UNDEFINED), error_msg(QString("")),
    data_log_filepath(boost::filesystem::path("./")), data_log_filestream(NULL),
    exec_log_filepath(boost::filesystem::path("./")), exec_log_filestream(NULL)
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
    data_log_timer.start();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::doWork()
{
    // ACTIONS TO DO DURING TRANSITIONS
    if(current_state != last_state)
    {
        uint8_t from_state = last_state;
        uint8_t to_state = current_state;

        last_state = current_state;

        if(from_state == UNDEFINED && to_state == STARTING)
        {
            initExecLogging();
        }

        printToExecLog(boost::str(boost::format("Transition from state %s to state %s\n")
                       % state_toString(from_state) % state_toString(to_state)));

        // TRANSITIONS ACTIONS TO DO
    }

    // ACTIONS TO DO IN CURRENT STATE
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
            printToExecLog("Starting xbee_setup...\n");
            if ((ret = xbee_setup(&xbee, "xbee1", "COM8", 57600)) != XBEE_ENONE) {
                setNewError(boost::str(boost::format("xbee_setup() error (%d : %s)\n") % ret % xbee_errorToStr(ret)));
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
            printToExecLog("Starting a new 64-bit bidirectionnal data connection to xbee...\n");
            if ((ret = xbee_conNew(xbee, &con, "64-bit Data", &drone_address)) != XBEE_ENONE) {
                setNewError(boost::str(boost::format("xbee_conNew() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
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
                    setNewError(boost::str(boost::format("xbee_conInfoGet() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
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
                        setNewError(boost::str(boost::format("xbee_conRx() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
                        current_state = EMERGENCY;
                        return;
                    }

                    if(pkt == NULL) break; // just for safety...

                    ++rx_packets_counter;
                    if(pkt->dataLen == sizeof(rxPacket)) {
                        struct rxPacket rx_data(pkt->data);

                        // DO SOMETHING WITH PACKETS
                        if(rx_data.packet_clock_ > max_packet_clock ||
                           ((int)max_packet_clock - (int)rx_data.packet_clock_) > 128) { // check if it's the most recent packet received or if it's a late packet
                            max_packet_clock = rx_data.packet_clock_;
                        }
                        if(min_packet_clock == max_packet_clock) {
                            // write packet to data
                        }
                        else if(rx_data.packet_clock_ != max_packet_clock) { // it's a late packet
                            printToExecLog(boost::str(boost::format("Late packet received (max_clock_counter = %d, packet_clock = %d\n")
                                                      % max_packet_clock % rx_data.packet_clock_));
                            packets_buffer.push_back(rx_data);

                            // check if the buffered packets can fill the data from min_packet_clock
                            processPacketsBuffer();
                        }

                        if ((ret = xbee_pktFree(pkt)) != XBEE_ENONE) {
                            setNewError(boost::str(boost::format("xbee_pktFree() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
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
                ui->timeEdit_data_log_time->setTime(QTime(0,0,0,data_log_timer.elapsed())); // why this doesn't work ?

            }
            break;
        }
        case DISCONNECTING:
        {
            statusBar_current_state.setText(QString("DISCONNECTING"));

            printToExecLog("Ending xbee connection...");
            if ((ret = xbee_conEnd(con)) != XBEE_ENONE) {
                setNewError(boost::str(boost::format("xbee_conEnd() error (%d : %s)\n") % ret % xbee_errorToStr(ret)).c_str());
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

            // FOR TESTING PURPOSES ONLY : TO BE REMOVED
            if (data_log_filestream) {
                printToDataLog("DATA\n");
                ui->timeEdit_data_log_time->setTime(QTime(0,0,0,0).addMSecs(data_log_timer.elapsed()));
                ui->doubleSpinBox_data_log_size->setValue(boost::filesystem::file_size(data_log_filepath)/1024.0); // 1 kb = 1024 bytes
            }

            break;
        }
        case UNDEFINED:
        default: // we're not supposed to be here
        {
            setNewError("Unexpected state");
            current_state = EMERGENCY;
            return;
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
        printToExecLog("Close event triggered : exiting\n");
        if(xbee_validate(xbee) == XBEE_ENONE) {
            current_state = DISCONNECTING;
            doWork();
            if ((ret = xbee_shutdown(xbee)) != XBEE_ENONE) {
                printToExecLog(boost::str(boost::format("xbee_shutdown() error (%d - %s)\n") % ret % xbee_errorToStr(ret)).c_str());
            }
        }

        closeDataLogging();
        closeExecLogging();
        event->accept();
    }
}

void MainWindow::on_pushButton_start_data_logging_clicked()
{
    printToExecLog("Start data logging button clicked...\n");

    initDataLogging();
    if(data_log_filestream)
    {
        std::string data_log_state_text = "Logging to : " + data_log_filename;
        data_log_filepath = boost::filesystem::path(data_log_filename);
        ui->lineEdit_data_log_state->setText(data_log_state_text.c_str());
        ui->pushButton_start_data_logging->setText("START NEW");
        ui->pushButton_stop_data_logging->setEnabled(true);
        ui->doubleSpinBox_data_log_size->setEnabled(true);
        data_log_timer = QTime(0,0,0,0);
        data_log_timer.start();
        ui->timeEdit_data_log_time->setTime(QTime(0,0,0,0));
        ui->timeEdit_data_log_time->setEnabled(true);
        return;
    }
}

void MainWindow::on_pushButton_stop_data_logging_clicked()
{
    printToExecLog("Stop data logging button clicked...\n");

    closeDataLogging();

    ui->lineEdit_data_log_state->setText("Idle");
    ui->pushButton_start_data_logging->setText("START");
    ui->pushButton_stop_data_logging->setEnabled(false);
    ui->doubleSpinBox_data_log_size->setValue(0);
    ui->doubleSpinBox_data_log_size->setEnabled(false);
    ui->timeEdit_data_log_time->setTime(QTime(0,0,0,0));
    ui->timeEdit_data_log_time->setEnabled(false);
}

std::string MainWindow::state_toString(uint8_t state_value) {
    switch(state_value)
    {
        case STARTING: return "STARTING";
        case CONNECTING: return "CONNECTING";
        case WORKING: return "WORKING";
        case DISCONNECTING: return "DISCONNECTING";
        case EMERGENCY: return "EMERGENCY";
        case UNDEFINED: return "UNDEFINED";
        default:
            error_msg = "state_toString() error (invalid state value)";
            current_state = EMERGENCY;
            return "-ERROR-";
    }
}

void MainWindow::initExecLogging() {
    if(exec_log_filestream) fclose(exec_log_filestream);
    exec_log_filestream = fopen("./exec_log.txt", "w");

    if(exec_log_filestream == NULL) {
        error_msg = "fopen() error (invalid exec_log_filestream handle)";
        current_state = EMERGENCY;
    }
}

void MainWindow::initDataLogging() {
    if(data_log_filestream) fclose(data_log_filestream);

    printToExecLog("Initializing data logging...\n");
    boost::filesystem::path data_log_dir_path("./" + ui->lineEdit_data_log_dir_path->text().toStdString());
    if(!boost::filesystem::exists(data_log_dir_path))
    {
        if (!boost::filesystem::create_directory(data_log_dir_path)) {
            error_msg = "create_directory() error";
            current_state = EMERGENCY;
            return;
        }
    }

    uint8_t data_log_number = 1;

    while(true) {
        data_log_filename = data_log_dir_path.string() + "/data" + boost::lexical_cast<std::string>((int)data_log_number) + ".txt";
        if(!boost::filesystem::exists(boost::filesystem::path(data_log_filename))
        ) {
            break;
        }
        ++data_log_number;

        if(data_log_number == 255) {
            error_msg = "data_log_filename infinite loop while ?";
            current_state = EMERGENCY;
            return;
        }
    }
    printToExecLog("Opening write filestream for data logging at : " + data_log_filename + "\n");
    data_log_filestream = fopen(data_log_filename.c_str(), "w");
    if (data_log_filestream == NULL) {
        error_msg = "fopen() error (invalid data_log_filestream handle)";
        current_state = EMERGENCY;
    }
}

void MainWindow::closeExecLogging() {
    if(exec_log_filestream) fclose(exec_log_filestream);
    exec_log_filestream = NULL;
}

void MainWindow::closeDataLogging() {
    if(data_log_filestream) {
        printToExecLog("Ending data logging...\n");
        fclose(data_log_filestream);
    }
    data_log_filestream = NULL;
}

void MainWindow::printToExecLog(std::string text)
{
    printToFile(exec_log_filestream, QTime::currentTime().toString("HH:mm:ss").toStdString() + "\t" + text);
}

void MainWindow::printToDataLog(std::string text) {
    printToFile(data_log_filestream, text);
}

void MainWindow::printToFile(FILE* filestream, std::string text)
{
    if(filestream)
    {
        fputs(text.c_str(), filestream);
        fflush(filestream);
    }
}

void MainWindow::processPacketToData(rxPacket pkt) {
    printToDataLog(boost::str(boost::format("%d\t%d\t%d\t%d\t%d\t%d\t%d")
                              % (int)pkt.packet_clock_
                              % (int)pkt.gyro_.roll_
                              % (int)pkt.gyro_.yaw_
                              % (int)pkt.gyro_.pitch_
                              % (int)pkt.accel_.longitudinal_
                              % (int)pkt.accel_.lateral_
                              % (int)pkt.accel_.vertical_));
}

void MainWindow::processPacketsBuffer() {
    bool found_min;
    while(true) {
        found_min = false;
        for(std::list<rxPacket>::iterator it = packets_buffer.begin(); it != packets_buffer.end(); ++it) {
            if(it->packet_clock_ == min_packet_clock) {
                printToExecLog(boost::str(boost::format("Latest missing packet_clock = %d present in packets_buffer, writing it to data_log...\n")
                                          % min_packet_clock));
                // It's the latest packet missing : we can write it to data_log, incremenet min_packet_clock, and remove it from the packets_buffer
                processPacketToData(*it);
                packets_buffer.erase(it);
                ++min_packet_clock;
                found_min = true;
                break; // we need to exit the loop because list size and iterator will change after removing the packet
            }
        }
        if(!found_min) {
            if(packets_buffer.size() == 10) {
                ++min_packet_clock; // we loop until we don't find any packet in the packets_buffer that verify the min_packet_clock value or until the packet_buffer is less then 10
                printToExecLog(boost::str(boost::format("The packets_buffer is full and we still miss the packet_clock = %d, ignoring it...\n")
                                      % min_packet_clock));
            }
            else break;
        }
    }
}

void MainWindow::setNewError(std::string text) {
    printToExecLog(text);
    error_msg = QString(text.c_str());
}

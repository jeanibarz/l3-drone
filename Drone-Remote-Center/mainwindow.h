#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <stdint.h>

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QLabel>

#include <boost/filesystem.hpp>

#include "libxbee3_v3.0.10/xbee.h"

enum STATE { UNDEFINED, STARTING, CONNECTING, WORKING, DISCONNECTING, EMERGENCY };

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer timer;
    QLabel statusBar_current_state;

    uint8_t counter;
    uint32_t rx_packets_counter;
    uint32_t tx_packets_counter;
    uint16_t latency_buffer[20];
    uint8_t latency_buffer_length;
    uint8_t latency_index;
    uint8_t current_state;
    uint8_t last_state;

    struct xbee *xbee;
    struct xbee_con *con;
    struct xbee_conAddress drone_address;
    xbee_err ret;
    QString error_msg;

    QTime data_log_timer;
    std::string data_log_filename;
    boost::filesystem::path data_log_filepath;
    FILE * data_log_filestream;

    boost::filesystem::path exec_log_filepath;
    FILE * exec_log_filestream;

    void closeEvent(QCloseEvent *event);
    uint16_t calculateAvgLatency();
    std::string state_toString(uint8_t state_value);
    void initExecLogging();
    void initDataLogging();

    void closeExecLogging();
    void closeDataLogging();

    void printToExecLog(std::string text);
    void printToDataLog(std::string text);
    void printToFile(FILE* filestream, std::string text);

    void setNewError(std::string text);

private slots:
    void doWork();
    void on_pushButton_start_data_logging_clicked();
    void on_pushButton_stop_data_logging_clicked();
};

#endif // MAINWINDOW_H

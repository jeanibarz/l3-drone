#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <stdint.h>

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QLabel>

#include "libxbee3_v3.0.10/xbee.h"

enum STATE { STARTING, CONNECTING, WORKING, DISCONNECTING, EMERGENCY };

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
    QTime log_timer;
    QLabel statusBar_current_state;

    uint8_t counter;
    uint32_t rx_packets_counter;
    uint32_t tx_packets_counter;
    uint16_t latency_buffer[20];
    uint8_t latency_buffer_length;
    uint8_t latency_index;
    uint8_t current_state;

    struct xbee *xbee;
    struct xbee_con *con;
    struct xbee_conAddress drone_address;
    xbee_err ret;
    QString error_msg;

    void closeEvent(QCloseEvent *event);
    uint16_t calculateAvgLatency();

private slots:
    void doWork();
};

#endif // MAINWINDOW_H

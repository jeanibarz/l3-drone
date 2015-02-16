#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <stdint.h>

#include "libxbee3_v3.0.10/xbee.h"

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
    uint8_t counter;
    uint32_t rx_packets_counter = 0;
    uint32_t tx_packets_counter = 0;
    uint16_t latency_buffer[20] = { 0 };
    uint8_t latency_buffer_length = 20;
    uint8_t latency_index = 0;

    struct xbee *xbee;
    struct xbee_con *con;
    struct xbee_conAddress drone_address;
    xbee_err ret;

    void closeEvent(QCloseEvent *event);
    uint16_t calculateAvgLatency();

private slots:
    xbee_err doWork();
};

#endif // MAINWINDOW_H

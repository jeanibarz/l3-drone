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

    struct xbee *xbee;
    struct xbee_con *con;
    struct xbee_conAddress drone_address;
    xbee_err ret;

private slots:
    xbee_err doWork();
};

#endif // MAINWINDOW_H

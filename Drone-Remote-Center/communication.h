#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <exception>
#include "stdint.h"

#include "libxbee3_v3.0.10/xbee.h"

struct Gyro
{
    uint16_t roll_; // P1
    uint16_t yaw_; // P2
    uint16_t pitch_; // P3
};

struct Accel
{
    uint16_t longitudinal_; // P4
    uint16_t lateral_; // P5
    uint16_t vertical_; // P6
};

/* rxPacket is the structure of the packets received from the Drone.
 * It contains every information we need to get about the Drone :
 * packet_clock number, drone state and sensors values. */
struct rxPacket
{
    uint8_t packet_clock_;
    uint8_t power_charge_;
    uint8_t drone_state_;
    struct Gyro gyro_;
    struct Accel accel_;

    rxPacket(uint8_t const * const data);
    void print();
};

/* txPacket is the structure of the packets to emit to the Drone.
 * It contains every information we need to send to the Drone :
 * the value of the 4 control channels. */
struct txPacket
{
    // uint8_t p1_5V;
    // uint8_t p2_0V;
    uint8_t p3_ail_; // control rolling
    uint8_t p4_ele_; // control pitch
    uint8_t p5_rud_; // control yaw
    uint8_t p6_thr_; // control elevation
    // uint8_t p7 gyr;
    // uint8_t p8_aux2;
    txPacket(uint8_t p3_ail, uint8_t p4_ele, uint8_t p5_rud, uint8_t p6_thr);
};

/* This is a function wrapper to easily send a txPacket */
xbee_err xbee_sendTxPacket(struct xbee_con * con, unsigned char * retVal, struct txPacket * tx_pkt);

#endif // COMMUNICATION


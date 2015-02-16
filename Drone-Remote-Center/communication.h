#ifndef _COMMUNICATION_H
#define _COMMUNICATION_H

#include <exception>
#include "stdint.h"

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

    rxPacket(uint8_t const * const data) {
        packet_clock_ = data[0];
        power_charge_ = data[1];
        drone_state_ = data[2];

        uint8_t gyro_index = 3;
        gyro_.roll_ = ( (uint16_t)data[gyro_index]<<8 ) + ( (uint16_t)data[gyro_index+1] );
        gyro_.yaw_ = ( (uint16_t)data[gyro_index+2]<<8 ) + ( (uint16_t)data[gyro_index+3] );
        gyro_.pitch_ = ( (uint16_t)data[gyro_index+4]<<8 ) + ( (uint16_t)data[gyro_index+5] );

        uint8_t accel_index = gyro_index + 6;
        accel_.longitudinal_ = ( (uint16_t)data[accel_index]<<8 ) + ( (uint16_t)data[accel_index+1] );
        accel_.lateral_ = ( (uint16_t)data[accel_index+2]<<8 ) + ( (uint16_t)data[accel_index+3] );
        accel_.vertical_ = ( (uint16_t)data[accel_index+4]<<8 ) + ( (uint16_t)data[accel_index+5] );
    };
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
    txPacket(uint8_t p3_ail, uint8_t p4_ele, uint8_t p5_rud, uint8_t p6_thr)
        : p3_ail_(p3_ail), p4_ele_(p4_ele), p5_rud_(p5_rud), p6_thr_(p6_thr) {};
};

/* This is a function wrapper to easily send a txPacket */
xbee_err xbee_sendTxPacket(struct xbee_con * con, unsigned char * retVal, struct txPacket * tx_pkt)
{
    xbee_connTx(con, retVal, (unsigned char *) tx_pkt, sizeof(*tx_pkt));
}

#endif // COMMUNICATION


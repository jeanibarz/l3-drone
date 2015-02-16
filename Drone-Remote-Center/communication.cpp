#include "communication.h"

rxPacket::rxPacket(uint8_t const * const data) {
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

txPacket::txPacket(uint8_t p3_ail, uint8_t p4_ele, uint8_t p5_rud, uint8_t p6_thr)
    : p3_ail_(p3_ail), p4_ele_(p4_ele), p5_rud_(p5_rud), p6_thr_(p6_thr)
{
};

xbee_err xbee_sendTxPacket(struct xbee_con * con, unsigned char * retVal, struct txPacket * tx_pkt)
{
    return xbee_connTx(con, retVal, (unsigned char *) tx_pkt, sizeof(*tx_pkt));
}

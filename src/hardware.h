/*
 * hardware.h
 *
 *  Created on: 8 июн. 2022 г.
 *      Author: yura
 */

#ifndef HARDWARE_H_
#define HARDWARE_H_

void buzzer_on(uint16_t duration);
int iec104_send_dio(CS104_Slave slave, uint16_t asdu_addr);
bool iec104_moxa_rcv_asdu(IMasterConnection connection, CS101_ASDU asdu);

#endif /* HARDWARE_H_ */

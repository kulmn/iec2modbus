/*
 * moxa_dev.h
 *
 *  Created on: 16 июн. 2020 г.
 *      Author: kulish_y
 */

#ifndef SRC_MOXA_DEV_H_
#define SRC_MOXA_DEV_H_


void moxa_buzzer(uint16_t duration);
int iec104_send_moxa_dio(CS104_Slave slave);
bool iec104_moxa_rcv_asdu(IMasterConnection connection, CS101_ASDU asdu);


#endif /* SRC_MOXA_DEV_H_ */

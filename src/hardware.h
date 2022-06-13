/*
 * hardware.h
 *
 *  Created on: 8 июн. 2022 г.
 *      Author: yura
 */

#ifndef HARDWARE_H_
#define HARDWARE_H_

#include "slog.h"


#ifdef MOXA_UC8410
#define DIO_NUM			4
#endif

#ifdef IRZ_RU21
#define GPIO_PATH			"/sys/class/gpio/IO_"
#define DIO_NUM			7
#endif

#ifdef X86_64
#define GPIO_PATH			"./sys/class/gpio/gpio"
#define DIO_NUM			7
#endif

#if	defined (X86_64) || defined (IRZ_RU21)
#define DIO_HIGH			1 // the DIO data is high
#define DIO_LOW			0 // the DIO data is low
int get_din_state(int diport, int *state);
#endif

void buzzer_on(uint16_t duration);
//int init_hardw_dio(void);
int iec104_send_dio(CS104_Slave slave, uint16_t asdu_addr);
bool iec104_moxa_rcv_asdu(IMasterConnection connection, CS101_ASDU asdu);

#endif /* HARDWARE_H_ */

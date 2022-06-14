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
#define DIO_HIGH			1 // the DIO data is high
#define DIO_LOW			0 // the DIO data is low
#define GPIO_PATH			"/sys/class/gpio/IO_"
#define DIO_NUM			7
static const uint8_t gpio_lst[] = {1,2,3,4,5,6,7};
int get_din_state(int diport, int *state);
#endif

#ifdef X86_64
#define DIO_HIGH			1 // the DIO data is high
#define DIO_LOW			0 // the DIO data is low
#define GPIO_PATH			"./sys/class/gpio/gpio"
#define DIO_NUM			7
static const uint8_t gpio_lst[] = {1,2,3,4,5,6,7};
int get_din_state(int diport, int *state);
#endif

#ifdef RUT955

#define 	DIO_HIGH		0		// high logic level = 0
#define 	DIO_LOW		1		// low logic level = 1
#define 	DIO_NUM		6		// 3 - inputs, 3 - output status
#define 	GPIO_PATH		"/sys/class/gpio/gpio"

#define		DIN1			21		// Digital input 	pins(1,6)
#define		DIN2			19		// Digital galvanically isolated input (0-4 VDC: low logic level / 9-30 VDC: high logic level)	pins(2,7)
#define		DIN3			2		// Digital non-isolated input (4 PIN connector)	pin 3 on 4 pin connector
#define		DOUT1			31		// Digital OC output
#define		DOUT2			32		// Relay output	pins(5,10)		(ON=1, OFF=0)
#define		DOUT3			35		// Digital open collector output (4 PIN connector)  pin 4 on 4 pin connector

#define		SIM				30
#define		MON			33
#define		MRST			34
#define		RS485_R			36
#define		SDCS			37
#define		HWRST			38

static const uint8_t gpio_lst[] = {DIN1, DIN2, DIN3, DOUT1, DOUT2, DOUT3};
int get_din_state(int diport, int *state);

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

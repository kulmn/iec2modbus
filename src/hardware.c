/*
 * hardware.c
 *
 *  Created on: 8 июн. 2022 г.
 *      Author: yura
 */


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "iec104_server.h"


#ifdef MOXA_UC8410
#include <moxadevice.h>
#include <sys/kd.h>
#include <sys/ioctl.h>
#endif


#if defined (X86_64)
//static const uint8_t gpio_lst[] = {0,0,0,0,0,0};
#endif


void buzzer_on(uint16_t duration)
{
#ifdef MOXA_UC8410
	uint16_t lw_freq = 100;
	unsigned int arg = ( (duration<<16) + lw_freq );
	int fd = open("/dev/console", O_RDWR);
	ioctl(fd, KDMKTONE, arg);
	close(fd);
#endif
}

/*
int init_hardw_dio(void)
{
#if	defined (X86_64) || defined (IRZ_RU21)
	char filename[64];

	for (int i = 1; i < DIO_NUM+1; i++)
	{
		char numb[8];
		snprintf(numb, 4, "%d", i);
		strcpy(filename, GPIO_PATH);
		strcat(filename, numb);
		strcat(filename, "/direction");

		int fd = open(filename, O_WRONLY );
		if (fd == -1)
		{
			slog_warn("Unable to open file %s ", filename );
			return 1;
		}

		if (write(fd, "in", 2 ) != 2)
		{
			slog_warn("Error writing to file %s ", filename );
			return 1;
		}
		close(fd);
	}
#endif
	return 0;
}
*/

int iec104_send_dio(CS104_Slave slave, uint16_t asdu_addr)
{
#if defined (MOXA_UC8410) || defined (IRZ_RU21) || defined (RUT955) || defined (X86_64)
	CS101_AppLayerParameters alParams;
	/* get the connection parameters - we need them to create correct ASDUs */
	alParams = CS104_Slave_getAppLayerParameters(slave );
	InformationObject io=NULL;

	CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, asdu_addr, false, false );
	int state=0;

	for (int i = 0; i < DIO_NUM; i++)
	{
		bool value;
		get_din_state(i, &state );
		if (state == DIO_HIGH) value = true;
		else value = false;
		io = (InformationObject) SinglePointInformation_create(NULL, i+1, value, IEC60870_QUALITY_GOOD );
		CS101_ASDU_addInformationObject(newAsdu, io );
		InformationObject_destroy(io );
	}
	CS104_Slave_enqueueASDU(slave, newAsdu );
	CS101_ASDU_destroy(newAsdu );
#endif

	return 0;
}

#if defined (X86_64) || defined (IRZ_RU21)  || defined (RUT955)
int get_din_state(int diport, int *state)
{
	char filename[64];
	char numb[8];

	//uint8_t dio_num = diport+1;
	uint8_t dio_num = gpio_lst[diport];
	snprintf(numb, 4, "%d", dio_num );
	strcpy(filename, GPIO_PATH );
	strcat(filename, numb );
	strcat(filename, "/value" );

	int fd = open(filename, O_RDONLY );
	if (fd == -1)
	{
		slog_warn("Unable to open file %s ", filename );
		return 1;
	}

	char buf[8];
	int num_rd = read(fd, buf, 4 );
	if (num_rd == -1)
	{
		slog_warn("Unable to read file %s ", filename );
		return 1;
	}

	close(fd);
	*state = strtol(buf, NULL, 10 );

	return 0;
}
#endif


bool iec104_moxa_rcv_asdu(IMasterConnection connection, CS101_ASDU asdu)
{
#ifdef MOXA_UC8410
	TypeID type_id = CS101_ASDU_getTypeID(asdu );
	uint8_t common_addr = CS101_ASDU_getCA(asdu );

	if ((type_id == C_SC_NA_1) && (common_addr == 255))// Single command
	{
		if (CS101_ASDU_getCOT(asdu ) == CS101_COT_ACTIVATION)
		{
			SingleCommand sc = (SingleCommand) CS101_ASDU_getElement(asdu, 0 );
			int ioa_addr = InformationObject_getObjectAddress((InformationObject) sc );
			int dig_port = ioa_addr - 1;
			if (dig_port >= 0 && dig_port < 4)
			{
				if (SingleCommand_getState(sc )) set_dout_state(dig_port, DIO_HIGH );
				else set_dout_state(dig_port, DIO_LOW );
			}

			CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON );
		} else CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT );
		IMasterConnection_sendASDU(connection, asdu );
		return true;
	}
#endif

	return false;
}


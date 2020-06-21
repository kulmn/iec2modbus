/*
 * moxa_dev.c
 *
 *  Created on: 16 июн. 2020 г.
 *      Author: kulish_y
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "iec104_server.h"
#include <fcntl.h>


#ifdef MOXA_UC8410
#include <moxadevice.h>
#include <sys/kd.h>
#include <sys/ioctl.h>
#include <unistd.h>

void moxa_buzzer(uint16_t duration)
{
	uint16_t lw_freq = 100;
	unsigned int arg = ( (duration<<16) + lw_freq );
	int fd = open("/dev/console", O_RDWR);
	ioctl(fd, KDMKTONE, arg);
	close(fd);
}

int iec104_send_moxa_dio(CS104_Slave slave)
{
	CS101_AppLayerParameters alParams;
	/* get the connection parameters - we need them to create correct ASDUs */
	alParams = CS104_Slave_getAppLayerParameters(slave );
	InformationObject io=NULL;

	CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 255, false, false );
	int state=0;

	for (int i = 0; i < 4; i++)
	{
		bool value;
		get_din_state(i, &state );
		if (state == DIO_HIGH) value = true;
		else value = false;
		io = (InformationObject) SinglePointInformation_create(NULL, i, value, IEC60870_QUALITY_GOOD );
		CS101_ASDU_addInformationObject(newAsdu, io );
		InformationObject_destroy(io );
	}
	CS104_Slave_enqueueASDU(slave, newAsdu );
	CS101_ASDU_destroy(newAsdu );
	return 0;
}


bool iec104_moxa_rcv_asdu(IMasterConnection connection, CS101_ASDU asdu)
{
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

	return false;
}


#else
void moxa_buzzer(uint16_t duration)
{

}

int iec104_send_moxa_dio(CS104_Slave slave)
{
	return 0;
}

bool iec104_moxa_rcv_asdu(IMasterConnection connection, CS101_ASDU asdu)
{
	return false;
}
#endif



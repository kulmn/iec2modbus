#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>



#include "iec104_server.h"
#include <byteswap.h>
#include <arpa/inet.h>
#include "slog.h"
#include <sys/time.h>
#include <time.h>
#include "libmodbus/modbus.h"




/* Callback handler to log sent or received messages (optional) */
static void rawMessageHandler(void* parameter, IMasterConnection conneciton, uint8_t* msg, int msgSize, bool sent)
{
    if (sent)      printf("SEND: ");
    else     printf("RCVD: ");

    for (int i = 0; i < msgSize; i++)   printf("%02x ", msg[i]);
    printf("\n");
}


/*************************************************************************************/
static int iec104_set_system_time(CP56Time2a newTime)
{
	struct timeval set_time;

	slog_info("Process time sync command with time %02i:%02i:%02i %02i/%02i/%04i ",
			CP56Time2a_getHour(newTime ), CP56Time2a_getMinute(newTime ),
			CP56Time2a_getSecond(newTime ), CP56Time2a_getDayOfMonth(newTime ),
			CP56Time2a_getMonth(newTime ), CP56Time2a_getYear(newTime ) + 2000 );

	uint64_t timestamp = CP56Time2a_toMsTimestamp(newTime);
	set_time.tv_sec = timestamp / 1000;
	set_time.tv_usec = timestamp % 1000;

	int rc = settimeofday(&set_time, NULL );
	if (rc == 0)
	{
		slog_info("Time sync successful.");
	} else
	{
		slog_warn("Time sync failed: %s ", strerror(errno) );
		return -1;
	}
	return rc;
}

int iec_104_single_point_asdu(Command_TypeDef *cmd, CS101_ASDU asdu)
{
	InformationObject io = NULL;
	uint16_t ioa = cmd->iec_ioa_addr;
	uint16_t data  =  *(uint16_t*) cmd->mem_ptr;
	bool sp_data;

	int size;

	QualityDescriptor quality=IEC60870_QUALITY_GOOD;
	if (cmd->mem_state == mem_err) quality = IEC60870_QUALITY_INVALID;

	if ((cmd->mb_func == MODBUS_FC_READ_COILS) || (cmd->mb_func == MODBUS_FC_READ_DISCRETE_INPUTS))
		size = cmd->mb_data_size;
	else
		size = cmd->iec_size;

	for (int i = 0; i < size; i++)
	{
		if ( data & (1<<i)  ) sp_data = true;
		else  sp_data = false;
		io = (InformationObject) SinglePointInformation_create(NULL, ioa++, sp_data, quality );
		InformationObject_setType(io, cmd->iec_func );
		CS101_ASDU_addInformationObject(asdu, io );
		InformationObject_destroy(io );
	}

	return 0;
}


int iec104_uint32_asdu(Command_TypeDef *cmd, CS101_ASDU asdu)
{
	if ((cmd->mb_func == MODBUS_FC_READ_HOLDING_REGISTERS) || (cmd->mb_func == MODBUS_FC_READ_INPUT_REGISTERS))
	{
		InformationObject io = NULL;
		uint32_t data;

		QualityDescriptor quality=IEC60870_QUALITY_GOOD;
		if (cmd->mem_state == mem_err) quality = IEC60870_QUALITY_INVALID;

		uint16_t ioa = cmd->iec_ioa_addr;
		uint8_t num_ioa = (cmd->mb_data_size * sizeof(uint16_t)) / (sizeof(uint32_t));
		uint16_t *data_ptr = (uint16_t*) cmd->mem_ptr;
		for (int i = 0; i < num_ioa; i++)
		{
			switch(cmd->add_params.byte_swap)
			{
				case cfg_btsw_abcd: data  = ntohl(((uint32_t)data_ptr[0] << 16) + data_ptr[1]); break;
				case cfg_btsw_badc: data  = ntohl((uint32_t)(bswap_16(data_ptr[0]) << 16) + bswap_16(data_ptr[1])); break;
				case cfg_btsw_cdab: data  = ntohl((((uint32_t)data_ptr[1]) << 16) + data_ptr[0]); break;
				default: data  = ntohl(bswap_32((((uint32_t)data_ptr[0]) << 16) + data_ptr[1]));
			}

			float value=0;
			memcpy(&value, &data, sizeof(uint32_t));

			io = (InformationObject) MeasuredValueShort_create(NULL, ioa++, value, quality);
//			io = (InformationObject) BitString32_create(NULL, ioa++, data );
			InformationObject_setType(io, cmd->iec_func );
			CS101_ASDU_addInformationObject(asdu, io );
			data_ptr += (sizeof(uint32_t));
			InformationObject_destroy(io );
		}
	} else
	{
		return 1; //FIXME error
	}
	return 0;
}

int iec104_uint16_asdu(Command_TypeDef *cmd, CS101_ASDU asdu)
{

	QualityDescriptor quality=IEC60870_QUALITY_GOOD;
	if (cmd->mem_state == mem_err) quality = IEC60870_QUALITY_INVALID;
	if ((cmd->mb_func == MODBUS_FC_READ_HOLDING_REGISTERS) || (cmd->mb_func == MODBUS_FC_READ_INPUT_REGISTERS))
	{
		InformationObject io = NULL;
		uint16_t ioa = cmd->iec_ioa_addr;
		uint8_t num_ioa = (cmd->mb_data_size * sizeof(uint16_t)) / (sizeof(uint16_t));
		uint16_t *data_ptr = (uint16_t*) cmd->mem_ptr;
		for (int i = 0; i < num_ioa; i++)
		{
			io = (InformationObject) MeasuredValueScaled_create(NULL, ioa++, data_ptr[i], quality );
			InformationObject_setType(io, cmd->iec_func );
			CS101_ASDU_addInformationObject(asdu, io );
			InformationObject_destroy(io );
		}
	} else
	{
		return 1; //FIXME error
	}
	return 0;
}



int iec104_create_asdu(Command_TypeDef *cmd, CS101_ASDU asdu)
{
	switch (cmd->iec_func)
	{
		case M_SP_NA_1:					// Single point information		1bit
		{
			if( iec_104_single_point_asdu(cmd,  asdu) ) return 1;

		}break;
//		case M_SP_TB_1:						//Single point information with time CP56Time2a

//		case M_DP_NA_1:					//Double point information
//		case M_DP_TB_1:					//Double point information with time CP56Time2a

		case M_ME_NA_1:						// normalized value	16bit
//		case M_ME_TD_1:						// normalized value with time CP56Time2a 16bit

		case M_ME_NB_1:						// scaled value	16bit
		{
			if(iec104_uint16_asdu(cmd,  asdu) !=0) return 1;
		}break;
//		case M_ME_TE_1:						// scaled value with time CP56Time2a 16bit

		case M_BO_NA_1:					//Bit string of 32 bit
		{
			if(iec104_uint32_asdu(cmd,  asdu) ) return 1;
		}break;
//		case M_BO_TB_1:					//Bit string of 32 bit with time CP56Time2a

		case M_ME_NC_1:						//  short floating point 32bit
		{
			if(iec104_uint32_asdu(cmd,  asdu) ) return 1;
		}break;
//		case M_ME_TF_1:					//  short floating point with time  CP56Time2a	32bit

		case M_IT_NA_1:						//   Integrated totals  32bit
		{
			if(iec104_uint32_asdu(cmd,  asdu) ) return 1;
		}break;

		default:
		{
			slog_error("Unsupported ASDU type id #: %d", cmd->iec_func );
			return 1;
		}
	}

	return 0;
}


int iec104_send_changed_data(CS104_Slave slave, Transl_Config_TypeDef *config, cfg_iec_prior priority)
{
	CS101_AppLayerParameters alParams;
	/* get the connection parameters - we need them to create correct ASDUs */
	alParams = CS104_Slave_getAppLayerParameters(slave );
	CS101_ASDU newAsdu;

	for (int i = 0; i < config->num_ports; i++)
	{
		for (int j = 0; j < config->serialport[i].num_slaves; j++)
		{
			int ca_addr = config->serialport[i].mb_slave[j].mb_slave_addr;
			for (int x = 0; x < config->serialport[i].mb_slave[j].read_cmnds_num; x++)
			{
				data_state	mem_state = config->serialport[i].mb_slave[j].read_cmnds[x].mem_state;
				cfg_iec_prior  iec_priority = config->serialport[i].mb_slave[j].read_cmnds[x].add_params.priority;

				if (    ( (mem_state == mem_chg || mem_state == mem_err ) && iec_priority == cfg_prior_hight )
						|| (iec_priority == cfg_prior_low) )
				{
					newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, ca_addr, false, false );
					if (iec104_create_asdu(&config->serialport[i].mb_slave[j].read_cmnds[x], newAsdu ) == 0)
					{
						CS104_Slave_enqueueASDU(slave, newAsdu );
						if (mem_state != mem_err)
							config->serialport[i].mb_slave[j].read_cmnds[x].mem_state = mem_cur;
/*
						InformationObject io = CS101_ASDU_getElement(newAsdu, 0 );
						int ioa_addr = InformationObject_getObjectAddress(io );
						slog_debug("Send data:  ID=%s, CA=%d, IOA=%d",
									TypeID_toString(config->serialport[i].mb_slave[j].read_cmnds[x].iec_func),
									config->serialport[i].mb_slave[j].mb_slave_addr, ioa_addr	);
*/
					}
					CS101_ASDU_destroy(newAsdu );
				}
			}
		}
	}

	return 0;
}

/*************************************************************************************/
bool iec104_receive_single_cmd(Command_TypeDef* cmd,  InformationObject io)
{
	SingleCommand sc = (SingleCommand) io;
	uint16_t *data_ptr = (uint16_t *) cmd->mem_ptr;

	if (SingleCommand_getState(sc ) )
	{
		if (cmd->add_params.set_params & (1 << iec_on_value) )
			data_ptr[0] = cmd->add_params.on_value;
		else
			data_ptr[0] = 1;
	}else
	{
		if (cmd->add_params.set_params & (1 << iec_off_value) )
			data_ptr[0] = cmd->add_params.off_value;
		else
			data_ptr[0] = 0;
	}
	cmd->mem_state = mem_new;
	return true;
}

/*************************************************************************************/
void iec104_receive_double_cmd(Command_TypeDef* cmd,  InformationObject io)
{
	DoublePointValue state = DoubleCommand_getState((DoubleCommand) io );
	uint16_t *data_ptr = (uint16_t *) cmd->mem_ptr;
	switch(state)
	{
		case IEC60870_DOUBLE_POINT_ON:
		{
			data_ptr[0] = 0xFFFF;
			cmd->mem_state = mem_new;
		}break;
		case IEC60870_DOUBLE_POINT_OFF:
		{
			data_ptr[0] = 0x0000;
			cmd->mem_state = mem_new;
		}break;
		default:
			cmd->mem_state = mem_err;
	}
}


/*************************************************************************************/
void iec104_receive_uint16(Command_TypeDef* cmd,  InformationObject io)
{
	uint16_t *data_ptr = (uint16_t *) cmd->mem_ptr;
	data_ptr[0] = (uint16_t ) SetpointCommandScaled_getValue((SetpointCommandScaled) io);
	cmd->mem_state = mem_new;
}

/*************************************************************************************/
void iec104_receive_uint32(Command_TypeDef* cmd,  InformationObject io)
{
	uint16_t *data_ptr = (uint16_t *) cmd->mem_ptr;
	uint32_t data = Bitstring32Command_getValue((Bitstring32Command) io);


	switch(cmd->add_params.byte_swap)
	{
		case cfg_btsw_abcd:
		{
			data  = ntohl(data);
			data_ptr[0] = (uint16_t) (data >> 16);
			data_ptr[1] = (uint16_t) (data);
		}break;
		case cfg_btsw_badc:
		{
			data  = ntohl(data);
			data_ptr[0] = bswap_16 ((uint16_t) (data) );
			data_ptr[1] = bswap_16 ((uint16_t) (data >> 16) );
		}break;
		case cfg_btsw_cdab:
		{
			data  = ntohl(data);
			data_ptr[0] = (uint16_t) (data);
			data_ptr[1] = (uint16_t) (data >> 16);
		}break;
		default:
		{
			data  = ntohl(bswap_32(data));
			data_ptr[0] = (uint16_t) (data >> 16);
			data_ptr[1] = (uint16_t) (data);
		}
	}

	cmd->mem_state = mem_new;

}


/*************************************************************************************/
bool iec104_receive_cmd(Command_TypeDef *cmd, InformationObject io, CS101_ASDU asdu)
{
	if (CS101_ASDU_getCOT(asdu ) == CS101_COT_ACTIVATION)
	{
		switch (cmd->iec_func)
		{
			case C_SC_NA_1: 				// Single command
			{
				if (! iec104_receive_single_cmd( cmd,   io))	return false;
			}break;
			case C_DC_NA_1: 				// Double command
			{
				iec104_receive_double_cmd( cmd,  io);
			}break;
			case C_SE_NA_1: 				// Setpoint command, normalized value
			{
				iec104_receive_uint16( cmd,   io);
			}break;
			case C_SE_NB_1: 				// Setpoint command, scaled value
			{
				iec104_receive_uint16( cmd,   io);
			}break;
			case C_SE_NC_1: 				// Setpoint command, short floating point value
			{
				iec104_receive_uint32( cmd,   io);
			}break;
			case C_BO_NA_1: 				// Bit string 32 bit
			{
				iec104_receive_uint32( cmd,   io);
			}break;
			default: {

			}

		}
		CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON );

	} else CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT );

	return true;
}


/*************************************************************************************/
static Command_TypeDef* find_iec_cmd(Transl_Config_TypeDef *config, InformationObject io ,TypeID type_id, int common_addr, uint8_t N_cmd)
{
	int ioa_addr = InformationObject_getObjectAddress(io );
	Command_TypeDef *find_cmd_ptr[MAX_CMD_CNT] ;
	uint8_t cmd_cnt = 0;

	for (int i = 0; i < MAX_CMD_CNT; i++)	find_cmd_ptr[i] = NULL;

	for (int i = 0; i < config->num_ports; i++)
	{
		for (int j = 0; j < config->serialport[i].num_slaves; j++)
		{
			uint8_t mb_addr =  config->serialport[i].mb_slave[j].mb_slave_addr;
			for (int x = 0;x < config->serialport[i].mb_slave[j].write_cmnds_num;  x++)	// Modbus slave read commands num
			{
				if ( (common_addr == mb_addr) && (ioa_addr == config->serialport[i].mb_slave[j].write_cmnds[x].iec_ioa_addr) && \
						(type_id == config->serialport[i].mb_slave[j].write_cmnds[x].iec_func) )
				{
					find_cmd_ptr[cmd_cnt++] = &config->serialport[i].mb_slave[j].write_cmnds[x];
					if (cmd_cnt >= MAX_CMD_CNT) return NULL;
				}
			}
		}
	}

	if (N_cmd >=MAX_CMD_CNT) N_cmd = 0;

	return find_cmd_ptr[N_cmd];
}


/*************************************************************************************
static bool clockSyncHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
{
	slog_info("Process time sync command with time %02i:%02i:%02i %02i/%02i/%04i ",
			CP56Time2a_getHour(newTime ), CP56Time2a_getMinute(newTime ),
			CP56Time2a_getSecond(newTime ), CP56Time2a_getDayOfMonth(newTime ),
			CP56Time2a_getMonth(newTime ), CP56Time2a_getYear(newTime ) + 2000 );

	// Set time for ACT_CON message
	CP56Time2a_setFromMsTimestamp(newTime, Hal_getTimeInMs() );
	// update system time here
//	if( iec104_set_system_time( newTime) != 0 ) return false;

	return true;
}
*/

/*************************************************************************************/
static bool iec104_set_time(IMasterConnection connection, CS101_ASDU asdu)
{
	if (CS101_ASDU_getCOT(asdu ) == CS101_COT_ACTIVATION)
	{
		ClockSynchronizationCommand csc = (ClockSynchronizationCommand) CS101_ASDU_getElement(asdu, 0 );
		CP56Time2a new_time = ClockSynchronizationCommand_getTime(csc );
		iec104_set_system_time(new_time );
		CP56Time2a_setFromMsTimestamp(new_time, Hal_getTimeInMs() );
		CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON );
	} else
		CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT );

	IMasterConnection_sendASDU(connection, asdu );
	return true;
}



static bool interrogationHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
	Transl_Config_TypeDef *config = parameter;
	CS101_ASDU newAsdu = NULL;
	CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection );

	slog_info("Received interrogation for group %i", qoi);

	if (qoi == IEC60870_QOI_STATION)
	{ // only handle station interrogation
		IMasterConnection_sendACT_CON(connection, asdu, false );

		for (int i = 0; i < config->num_ports; i++)
		{
			for (int j = 0; j < config->serialport[i].num_slaves; j++)
			{
				uint8_t mb_addr =  config->serialport[i].mb_slave[j].mb_slave_addr;
				for (int x = 0;x < config->serialport[i].mb_slave[j].read_cmnds_num;  x++)	// Modbus slave read commands num
				{
					newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, mb_addr, false, false );
					if ( (config->serialport[i].mb_slave[j].read_cmnds[x].mem_state != mem_init) || \
							config->serialport[i].mb_slave[j].read_cmnds[x].mem_state != mem_err)
					{
						if (iec104_create_asdu(&config->serialport[i].mb_slave[j].read_cmnds[x],  newAsdu) == 0)
						{
							IMasterConnection_sendASDU(connection, newAsdu );
						}
					}
					CS101_ASDU_destroy(newAsdu );
				}
			}
		}

		IMasterConnection_sendACT_TERM(connection, asdu );
	} else
	{
		IMasterConnection_sendACT_CON(connection, asdu, true );
	}

	return true;
}





static bool asduHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu)
{
	Transl_Config_TypeDef *config = (Transl_Config_TypeDef*) parameter;
	bool	rc = false;

	Command_TypeDef *write_cmd = NULL;
	TypeID type_id = CS101_ASDU_getTypeID(asdu );
	uint8_t common_addr = CS101_ASDU_getCA(asdu );
	CS101_CauseOfTransmission cot = CS101_ASDU_getCOT(asdu);

	slog_debug("ASDU received:  ID=%s, CA=%d, COT=%s",
			TypeID_toString(type_id),  common_addr, CS101_CauseOfTransmission_toString(cot));

	if ( iec104_moxa_rcv_asdu(connection, asdu)) return true;			// moxa dio

	switch (type_id)
	{
		case C_CS_NA_1: /* 103 - Clock synchronization command */
		{
			rc = iec104_set_time( connection, asdu );

		}break;

		default:
		{
			int el_num = CS101_ASDU_getNumberOfElements(asdu );
			for (int i = 0; i < el_num; i++)
			{
				InformationObject io = CS101_ASDU_getElement(asdu, i );
				if (io == NULL)
				{
					slog_warn("Wrong ASDU received");
					return false;
				}

				for (uint8_t cmd_num=0; cmd_num < MAX_CMD_CNT; cmd_num++)
				{
					write_cmd = find_iec_cmd(config, io, type_id, common_addr , cmd_num);
					if (write_cmd != NULL)
					{
						slog_debug("command found: ioa = %d, ca = %d, type_id = %d ", write_cmd->iec_ioa_addr, common_addr, write_cmd->iec_func );
						if (iec104_receive_cmd(write_cmd, io, asdu ))
						{
							IMasterConnection_sendASDU(connection, asdu );
							rc = true;
							cmd_num = MAX_CMD_CNT;
						}
					}
				}
				InformationObject_destroy(io );
			}
		}
	}

	return rc;
}

static bool connectionRequestHandler(void *parameter, const char *ipAddress)
{
	slog_info("New connection request from %s", ipAddress );

#if 0
    if (strcmp(ipAddress, "127.0.0.1") == 0) {
        printf("Accept connection\n");
        return true;
    }
    else {
        printf("Deny connection\n");
        return false;
    }
#else
	return true;
#endif
}

static void connectionEventHandler(void* parameter, IMasterConnection con, CS104_PeerConnectionEvent event)
{
    if (event == CS104_CON_EVENT_CONNECTION_OPENED) {
        slog_info("Connection opened (%p)", con );
    }
    else if (event == CS104_CON_EVENT_CONNECTION_CLOSED) {
        slog_info( "Connection closed (%p)", con);
    }
    else if (event == CS104_CON_EVENT_ACTIVATED) {
        slog_info("Connection activated (%p)", con );
    }
    else if (event == CS104_CON_EVENT_DEACTIVATED) {
        slog_info("Connection deactivated (%p)", con );
    }
}




CS104_Slave iec104_server_init( Transl_Config_TypeDef *config, bool debug )
{
	CS104_Slave slave = NULL;


	Lib60870_enableDebugOutput(true);

	/* create a new slave/server instance with default connection parameters and
	 * default message queue size */
	slave = CS104_Slave_create(100, 100 );
	CS104_Slave_setLocalAddress(slave, "0.0.0.0" );
	/* Set mode to a single redundancy group
	 * NOTE: library has to be compiled with CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP enabled (=1)	 */
//	CS104_Slave_setServerMode(slave, CS104_MODE_SINGLE_REDUNDANCY_GROUP );
	CS104_Slave_setServerMode(slave, CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP );
	/* set the callback handler for the clock synchronization command */
//	CS104_Slave_setClockSyncHandler(slave, clockSyncHandler, NULL );
	/* set the callback handler for the interrogation command */
	CS104_Slave_setInterrogationHandler(slave, interrogationHandler, config );
	/* set handler for other message types */
	CS104_Slave_setASDUHandler(slave, asduHandler, config );
	/* set handler to handle connection requests (optional) */
	CS104_Slave_setConnectionRequestHandler(slave, connectionRequestHandler, NULL );
	/* set handler to track connection events (optional) */
	CS104_Slave_setConnectionEventHandler(slave, connectionEventHandler, NULL );
	/* uncomment to log messages */
	if (debug)
	CS104_Slave_setRawMessageHandler(slave, rawMessageHandler, NULL);

	return slave;
}


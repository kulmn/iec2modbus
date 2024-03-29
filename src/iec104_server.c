
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



typedef struct  {
	char*		func_str;
	TypeID		func_n;
} cfg_iec_func;

TypeID String_to_TypeID(const char *str)
{
	const cfg_iec_func iec_fn_str[] = {
	{"M_SP_NA_1", M_SP_NA_1}, {"M_SP_TA_1", M_SP_TA_1}, {"M_DP_NA_1", M_DP_NA_1}, {"M_DP_TA_1", M_DP_TA_1},
	{"M_ST_NA_1", M_ST_NA_1}, {"M_ST_TA_1", M_ST_TA_1}, {"M_BO_NA_1", M_BO_NA_1}, {"M_BO_TA_1", M_BO_TA_1},
	{"M_ME_NA_1", M_ME_NA_1}, {"M_ME_TA_1", M_ME_TA_1}, {"M_ME_NB_1", M_ME_NB_1}, {"M_ME_TB_1", M_ME_TB_1},
	{"M_ME_NC_1", M_ME_NC_1}, {"M_ME_TC_1", M_ME_TC_1}, {"M_IT_NA_1", M_IT_NA_1}, {"M_IT_TA_1", M_IT_TA_1},
	{"M_EP_TA_1", M_EP_TA_1}, {"M_EP_TB_1", M_EP_TB_1}, {"M_EP_TC_1", M_EP_TC_1}, {"M_PS_NA_1", M_PS_NA_1},
	{"M_ME_ND_1", M_ME_ND_1}, {"M_SP_TB_1", M_SP_TB_1}, {"M_DP_TB_1", M_DP_TB_1}, {"M_ST_TB_1", M_ST_TB_1},
	{"M_BO_TB_1", M_BO_TB_1}, {"M_ME_TD_1", M_ME_TD_1}, {"M_ME_TE_1", M_ME_TE_1}, {"M_ME_TF_1", M_ME_TF_1},
	{"M_IT_TB_1", M_IT_TB_1}, {"M_EP_TD_1", M_EP_TD_1}, {"M_EP_TE_1", M_EP_TE_1}, {"M_EP_TF_1", M_EP_TF_1},

	{"C_SC_NA_1", C_SC_NA_1}, {"C_DC_NA_1", C_DC_NA_1}, {"C_RC_NA_1", C_RC_NA_1}, {"C_SE_NA_1", C_SE_NA_1},
	{"C_SE_NB_1", C_SE_NB_1}, {"C_SE_NC_1", C_SE_NC_1}, {"C_BO_NA_1", C_BO_NA_1}, {"C_SC_TA_1", C_SC_TA_1},
	{"C_DC_TA_1", C_DC_TA_1}, {"C_RC_TA_1", C_RC_TA_1},	{"C_SE_TA_1", C_SE_TA_1}, {"C_SE_TB_1", C_SE_TB_1},
	{"C_SE_TC_1", C_SE_TC_1}, {"C_BO_TA_1", C_BO_TA_1}, {"C_IC_NA_1", C_IC_NA_1},	{"C_CI_NA_1", C_CI_NA_1},
	{"C_RD_NA_1", C_RD_NA_1}, {"C_CS_NA_1", C_CS_NA_1}, {"C_TS_NA_1", C_TS_NA_1}, {"C_RP_NA_1", C_RP_NA_1},
	{"C_CD_NA_1", C_CD_NA_1}, {"C_TS_TA_1", C_TS_TA_1},
	};

	TypeID iec_func=0;

	size_t iec_fn_str_len = sizeof(iec_fn_str) / sizeof(iec_fn_str[0]);

	for (int fn=0; fn< iec_fn_str_len; fn++)
	{
		if ( !strcmp(str, iec_fn_str[fn].func_str) ) iec_func = iec_fn_str[fn].func_n;
	}

	return iec_func;
}


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

int iec_104_single_point_asdu(iec104_command *cmd, CS101_ASDU asdu)
{
	InformationObject io = NULL;
	uint32_t data, bitmask;
//	uint16_t data  =  *(uint16_t*) cmd->value->mem_ptr;
	bool sp_data;


	QualityDescriptor quality=IEC60870_QUALITY_GOOD;
	if (cmd->value->mem_state == mem_err) quality = IEC60870_QUALITY_INVALID;


	if (cmd->value->mem_size > sizeof(uint32_t))
	{
		return 1;		//FIXME error, max 32 bits
	}

	if (cmd->value->mem_size == sizeof(uint32_t))
	{
		uint16_t *data_ptr = (uint16_t*) cmd->value->mem_ptr;
		switch(cmd->add_params.byte_swap)
		{
			case cfg_btsw_abcd: data  = ntohl(((uint32_t)data_ptr[0] << 16) + data_ptr[1]); break;
			case cfg_btsw_badc: data  = ntohl((uint32_t)(bswap_16(data_ptr[0]) << 16) + bswap_16(data_ptr[1])); break;
			case cfg_btsw_cdab: data  = ntohl((((uint32_t)data_ptr[1]) << 16) + data_ptr[0]); break;
			default: data  = ntohl(bswap_32((((uint32_t)data_ptr[0]) << 16) + data_ptr[1]));
		}
	}else
	{
		memcpy(&data, cmd->value->mem_ptr, cmd->value->mem_size);
	}

	uint8_t num_bits;
	if (cmd->add_params.set_params & (1 << iec_bitmask) )
	{
		bitmask = cmd->add_params.bitmask;
		num_bits = (cmd->value->mem_size << 3);
	}else
	{
		bitmask = UINT32_MAX;
		num_bits = cmd->iec_size;
	}

	int ioa = cmd->iec_ioa_addr;
	for (int i = 0; i < num_bits; i++)
	{
		if (bitmask & (1<<i) )
		{
			if ( data & (1<<i)  ) sp_data = true;
			else  sp_data = false;
			io = (InformationObject) SinglePointInformation_create(NULL, ioa++, sp_data, quality );
			InformationObject_setType(io, cmd->iec_func );
			CS101_ASDU_addInformationObject(asdu, io );
			InformationObject_destroy(io );
		}
	}

/*
	if (size>0 && size<17)
	{
		for (int i = 0; i < size; i++)
		{
			if ( data & (1<<i)  ) sp_data = true;
			else  sp_data = false;
			io = (InformationObject) SinglePointInformation_create(NULL, ioa++, sp_data, quality );
			InformationObject_setType(io, cmd->iec_func );
			CS101_ASDU_addInformationObject(asdu, io );
			InformationObject_destroy(io );
		}
	}else
	{
		//FIXME Error
	}
*/
	return 0;
}


int iec104_uint32_asdu(iec104_command *cmd, CS101_ASDU asdu)
{
	uint8_t num_ioa = (cmd->value->mem_size) / (sizeof(uint32_t));
	if (num_ioa)
	{
		InformationObject io = NULL;
		uint32_t data;

		QualityDescriptor quality=IEC60870_QUALITY_GOOD;
		if (cmd->value->mem_state == mem_err) quality = IEC60870_QUALITY_INVALID;

		uint16_t ioa = cmd->iec_ioa_addr;
		uint16_t *data_ptr = (uint16_t*) cmd->value->mem_ptr;
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
			InformationObject_setType(io, cmd->iec_func );
			CS101_ASDU_addInformationObject(asdu, io );
			data_ptr += (sizeof(uint32_t) / sizeof(uint16_t) );
			InformationObject_destroy(io );
		}
	} else
	{
		return 1; //FIXME error
	}
	return 0;
}

int iec104_uint16_asdu(iec104_command *cmd, CS101_ASDU asdu)
{

	QualityDescriptor quality=IEC60870_QUALITY_GOOD;
	if (cmd->value->mem_state == mem_err) quality = IEC60870_QUALITY_INVALID;

	uint8_t num_ioa = (cmd->value->mem_size) / (sizeof(uint16_t));
	if (num_ioa)
	{
		InformationObject io = NULL;
		uint16_t ioa = cmd->iec_ioa_addr;
		uint16_t *data_ptr = (uint16_t*) cmd->value->mem_ptr;
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



int iec104_create_asdu(iec104_command *cmd, CS101_ASDU asdu)
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


int iec104_send_changed_data( iec104_server *config, cfg_iec_prior priority)
{
	CS101_AppLayerParameters alParams;
	/* get the connection parameters - we need them to create correct ASDUs */
	alParams = CS104_Slave_getAppLayerParameters(config->server );
	CS101_ASDU newAsdu;

	for (int j = 0; j < config->iec104_slave_num; j++)
	{
		int ca_addr = config->iec104_slave[j].iec_asdu_addr;
		for (int x = 0; x < config->iec104_slave[j].iec104_read_cmd_num;  x++)
		{
			pthread_mutex_lock(&config->iec104_slave[j].iec104_read_cmds[x].value->lock);
			data_state mem_state = config->iec104_slave[j].iec104_read_cmds[x].value->mem_state;
			cfg_iec_prior iec_priority = config->iec104_slave[j].iec104_read_cmds[x].add_params.priority;

			if (  ((mem_state == mem_chg) && priority == cfg_prior_hight)					// changed data
					|| ((mem_state != mem_init) && iec_priority == cfg_prior_hight)		// cfg_prior_hight in conf file
					|| (mem_state != mem_init && priority == cfg_prior_low)  )			// all data
			{
				newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, ca_addr, false, false );
				if (iec104_create_asdu(&config->iec104_slave[j].iec104_read_cmds[x], newAsdu ) == 0)
				{
					CS104_Slave_enqueueASDU(config->server, newAsdu );
					if (mem_state != mem_err) config->iec104_slave[j].iec104_read_cmds[x].value->mem_state = mem_cur;
				}
				CS101_ASDU_destroy(newAsdu );
			}
			pthread_mutex_unlock(&config->iec104_slave[j].iec104_read_cmds[x].value->lock);
		}
	}
	return 0;
}

/*************************************************************************************/
bool iec104_receive_single_cmd(iec104_command* cmd,  InformationObject io)
{
	SingleCommand sc = (SingleCommand) io;
	uint32_t data;

	if (cmd->add_params.set_params & ( (1 << iec_on_value) | (1 << iec_off_value) ))
	{
		if (SingleCommand_getState(sc ) )
		{
			if (cmd->add_params.set_params & (1 << iec_on_value) )
			{
				data = cmd->add_params.on_value;
				cmd->value->mem_state = mem_new;
			}else return false;
		}else
		{
			if (cmd->add_params.set_params & (1 << iec_off_value) )
			{
				data = cmd->add_params.off_value;
				cmd->value->mem_state = mem_new;
			}else return false;
		}
	}else
	{
		if (SingleCommand_getState(sc ) )		data = 1;
		else data = 0;
		cmd->value->mem_state = mem_new;
	}

	uint16_t *data_ptr = (uint16_t*) cmd->value->mem_ptr;
	if (cmd->value->mem_size == sizeof(uint32_t))
	{
		switch (cmd->add_params.byte_swap)
		{
			case cfg_btsw_abcd: {
				data = ntohl(data );
				data_ptr[0] = (uint16_t) (data >> 16);
				data_ptr[1] = (uint16_t) (data);
			}
				break;
			case cfg_btsw_badc: {
				data = ntohl(data );
				data_ptr[0] = bswap_16((uint16_t ) (data) );
				data_ptr[1] = bswap_16((uint16_t ) (data >> 16) );
			}
				break;
			case cfg_btsw_cdab: {
				data = ntohl(data );
				data_ptr[0] = (uint16_t) (data);
				data_ptr[1] = (uint16_t) (data >> 16);
			}
				break;
			default: {
				data = ntohl(bswap_32(data ) );
				data_ptr[0] = (uint16_t) (data >> 16);
				data_ptr[1] = (uint16_t) (data);
			}
		}
	} else
	{
		data_ptr[0] = data;
	}


	return true;
}

/*************************************************************************************/
void iec104_receive_double_cmd(iec104_command* cmd,  InformationObject io)
{
	DoublePointValue state = DoubleCommand_getState((DoubleCommand) io );
	uint16_t *data_ptr = (uint16_t *) cmd->value->mem_ptr;
	switch(state)
	{
		case IEC60870_DOUBLE_POINT_ON:
		{
			data_ptr[0] = 0xFFFF;
			cmd->value->mem_state = mem_new;
		}break;
		case IEC60870_DOUBLE_POINT_OFF:
		{
			data_ptr[0] = 0x0000;
			cmd->value->mem_state = mem_new;
		}break;
		default:
			cmd->value->mem_state = mem_err;
	}
}


/*************************************************************************************/
void iec104_receive_uint16(iec104_command* cmd,  InformationObject io)
{
	uint16_t *data_ptr = (uint16_t *) cmd->value->mem_ptr;
	data_ptr[0] = (uint16_t ) SetpointCommandScaled_getValue((SetpointCommandScaled) io);
	cmd->value->mem_state = mem_new;
}

/*************************************************************************************/
void iec104_receive_uint32(iec104_command* cmd,  InformationObject io)
{
	uint16_t *data_ptr = (uint16_t *) cmd->value->mem_ptr;
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

	cmd->value->mem_state = mem_new;
}


/*************************************************************************************/
bool iec104_receive_cmd(iec104_command *cmd, InformationObject io, CS101_ASDU asdu)
{
	if (CS101_ASDU_getCOT(asdu ) == CS101_COT_ACTIVATION)
	{
		pthread_mutex_lock(&cmd->value->lock);
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
		pthread_mutex_unlock(&cmd->value->lock);

	} else CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT );

	return true;
}


/*************************************************************************************/
static iec104_command* find_iec_cmd(iec104_server *config, InformationObject io, TypeID type_id, int common_addr, uint8_t N_cmd)
{
	int ioa_addr = InformationObject_getObjectAddress(io );
	iec104_command *find_cmd_ptr[MAX_CMD_CNT];
	uint8_t cmd_cnt = 0;

	for (int i = 0; i < MAX_CMD_CNT; i++)		find_cmd_ptr[i] = NULL;

	for (int j = 0; j < config->iec104_slave_num; j++)
	{
		uint8_t asdu_addr = config->iec104_slave[j].iec_asdu_addr;
		for (int x = 0; x < config->iec104_slave[j].iec104_write_cmd_num;  x++)	// slave write commands number
		{
			if ((common_addr == asdu_addr) && (ioa_addr == config->iec104_slave[j].iec104_write_cmds[x].iec_ioa_addr) && (type_id == config->iec104_slave[j].iec104_write_cmds[x].iec_func))
			{
				find_cmd_ptr[cmd_cnt++] = &config->iec104_slave[j].iec104_write_cmds[x];
				if (cmd_cnt >= MAX_CMD_CNT) return NULL;
			}
		}
	}
	if (N_cmd >= MAX_CMD_CNT) N_cmd = 0;

	return find_cmd_ptr[N_cmd];
}


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



static bool interrogation_command(iec104_server *config, IMasterConnection connection, CS101_ASDU asdu)
{
	CS101_ASDU newAsdu = NULL;
	CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection );

	InterrogationCommand irc = (InterrogationCommand) CS101_ASDU_getElement(asdu, 0 );
	if (irc == NULL)
	{
		slog_warn("Wrong ASDU received" );
		return false;
	}
	uint8_t qoi = InterrogationCommand_getQOI(irc );

	slog_info("Received interrogation for group %i", qoi );

	if (qoi == IEC60870_QOI_STATION)
	{ // only handle station interrogation
		IMasterConnection_sendACT_CON(connection, asdu, false );

		for (int j = 0; j < config->iec104_slave_num; j++)
		{
			uint8_t mb_addr = config->iec104_slave[j].iec_asdu_addr;
			for (int x = 0; x < config->iec104_slave[j].iec104_read_cmd_num;  x++)	// slave read commands number
			{
				pthread_mutex_lock(&config->iec104_slave[j].iec104_read_cmds[x].value->lock);
				newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, mb_addr, false, false );
				if ((config->iec104_slave[j].iec104_read_cmds[x].value->mem_state != mem_init) || config->iec104_slave[j].iec104_read_cmds[x].value->mem_state != mem_err)
				{
					if (iec104_create_asdu(&config->iec104_slave[j].iec104_read_cmds[x], newAsdu ) == 0)
						IMasterConnection_sendASDU(connection, newAsdu );
				}
				CS101_ASDU_destroy(newAsdu );
				pthread_mutex_unlock(&config->iec104_slave[j].iec104_read_cmds[x].value->lock);
			}
		}

		IMasterConnection_sendACT_TERM(connection, asdu );
	} else
		IMasterConnection_sendACT_CON(connection, asdu, true );

	return true;
}



static bool asduHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu)
{
	iec104_server *config = (iec104_server*) parameter;
	bool	rc = false;

	iec104_command *write_cmd = NULL;
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

		case C_IC_NA_1:  /* 100 - interrogation command */
		{
			rc = interrogation_command(config, connection,  asdu);
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


void iec104_add_slave( iec104_server *srv, uint16_t asdu_addr )
{
	iec104_slave *new_ptr = NULL;
	uint16_t slave_num = srv->iec104_slave_num++;

	new_ptr = (iec104_slave*) malloc(srv->iec104_slave_num * sizeof(iec104_slave) );			//FIXME add malloc error
	for(int i=0; i< slave_num; i++)		new_ptr[i] = srv->iec104_slave[i];

	if (srv->iec104_slave != NULL) 		free(srv->iec104_slave);
	srv->iec104_slave = new_ptr;
	srv->iec104_slave[srv->iec104_slave_num-1].iec_asdu_addr = asdu_addr;
}

iec104_command* iec104_add_slave_rd_cmd( iec104_slave *slave )
{
	iec104_command *new_ptr = NULL;
	uint16_t cmd_num = slave->iec104_read_cmd_num++;

	new_ptr = (iec104_command*) malloc(slave->iec104_read_cmd_num * sizeof(iec104_command) );		//FIXME add malloc error
	for(int i=0; i< cmd_num; i++)
		new_ptr[i] = slave->iec104_read_cmds[i];
	if (slave->iec104_read_cmds != NULL)
		free(slave->iec104_read_cmds);
	slave->iec104_read_cmds = new_ptr;

	return &slave->iec104_read_cmds[slave->iec104_read_cmd_num-1];
}

iec104_command*  iec104_add_slave_wr_cmd( iec104_slave *slave )
{
	iec104_command *new_ptr = NULL;
	uint16_t cmd_num = slave->iec104_write_cmd_num++;

	new_ptr = (iec104_command*) malloc(slave->iec104_write_cmd_num * sizeof(iec104_command) );	//FIXME add malloc error
	for(int i=0; i< cmd_num; i++)
		new_ptr[i] = slave->iec104_write_cmds[i];
	if (slave->iec104_write_cmds != NULL)
		free(slave->iec104_write_cmds);
	slave->iec104_write_cmds = new_ptr;

	return &slave->iec104_write_cmds[slave->iec104_write_cmd_num-1];
}



iec104_server* iec104_server_init( bool debug )
{
	CS104_Slave slave = NULL;
	iec104_server *iec_server = NULL;

	iec_server = (iec104_server*) malloc( sizeof(iec104_server) );
	if (iec_server == NULL)
	{
		slog_error("Unable to allocate %d bytes for iec104 server ", sizeof(iec104_server) );
		return NULL;
	}

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
//	CS104_Slave_setInterrogationHandler(slave, interrogationHandler, config );
	/* set handler for other message types */
	CS104_Slave_setASDUHandler(slave, asduHandler, iec_server );
	/* set handler to handle connection requests (optional) */
	CS104_Slave_setConnectionRequestHandler(slave, connectionRequestHandler, NULL );
	/* set handler to track connection events (optional) */
	CS104_Slave_setConnectionEventHandler(slave, connectionEventHandler, NULL );
	/* uncomment to log messages */
	if (debug)
	CS104_Slave_setRawMessageHandler(slave, rawMessageHandler, NULL);

	iec_server->server = slave;
	iec_server->iec104_slave_num = 0;
	iec_server->iec104_slave = NULL;
	iec_server->iec104_send_rate = 30;

	return iec_server;
}

void iec104_server_stop( iec104_server *srv )
{
	CS104_Slave_stop(srv->server );
	CS104_Slave_destroy(srv->server );

	// free memory
	for (int i = 0; i < srv->iec104_slave_num; i++)
	{
		for (int x = 0; x < srv->iec104_slave[i].iec104_read_cmd_num; x++)
		{
			free(srv->iec104_slave[i].iec104_read_cmds[x].value->mem_ptr);
			free(srv->iec104_slave[i].iec104_read_cmds[x].value);
		}
		for (int x = 0; x < srv->iec104_slave[i].iec104_write_cmd_num; x++)
		{
			free(srv->iec104_slave[i].iec104_write_cmds[x].value->mem_ptr);
			free(srv->iec104_slave[i].iec104_write_cmds[x].value);
		}

		free(srv->iec104_slave[i].iec104_read_cmds);
		free(srv->iec104_slave[i].iec104_write_cmds);
	}

	free(srv->iec104_slave);
	free(srv);
}



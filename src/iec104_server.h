/*
 * iec104_server.h
 *
 *  Created on: 25 мая 2020 г.
 *      Author: kulish_y
 */

#ifndef IEC104_SERVER_H_
#define IEC104_SERVER_H_

//#include "ext_configs.h"
#include <errno.h>

#include "cs104_slave.h"
#include "hal_time.h"
#include "moxa_dev.h"
#include "memory.h"

#define MAX_CMD_CNT		4


typedef enum
{
	cfg_btsw_dcba = 0,
	cfg_btsw_abcd,
	cfg_btsw_badc,
	cfg_btsw_cdab
} cfg_byte_swap;

typedef enum  {
	cfg_prior_low =0,
	cfg_prior_hight,
} cfg_iec_prior;

typedef enum
{
	iec_priority = 0,
	iec_byteswap,
	iec_bitmask,
	iec_on_value,
	iec_off_value
} iec_set_params_flags;

typedef struct  {
	uint32_t				set_params;
	uint32_t				on_value;
	uint32_t				off_value;
	cfg_iec_prior		priority;
	cfg_byte_swap		byte_swap;
	uint32_t				bitmask;
} iec_add_params;

typedef struct  {
	TypeID				iec_func;
	uint16_t				iec_ioa_addr;
	uint8_t				iec_size;
	iec_add_params		add_params;
	data_mem			*value;
} iec104_command;

typedef struct  {
	uint16_t					iec_asdu_addr;
	uint8_t					iec104_read_cmd_num;
	iec104_command 		*iec104_read_cmds;
	uint8_t					iec104_write_cmd_num;
	iec104_command 		*iec104_write_cmds;
} iec104_slave;

typedef struct  {
	CS104_Slave 				server;
	uint16_t						iec104_send_rate;
	uint16_t						iec104_slave_num;
	iec104_slave					*iec104_slave;
} iec104_server;





CS104_Slave iec104_server_init( iec104_server *config, bool debug );
int iec104_send_changed_data(CS104_Slave slave, iec104_server *config, cfg_iec_prior priority);
//void iec104_server_start(void);
void iec104_server_stop( iec104_server *srv );
void iec104_add_slave( iec104_server *srv, uint16_t asdu_addr );
iec104_command* iec104_add_slave_rd_cmd( iec104_slave *slave );
TypeID String_to_TypeID(const char *str);

#endif /* IEC104_SERVER_H_ */

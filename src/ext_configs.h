/*
 * ext_configs.h
 *
 *  Created on: 23 мая 2020 г.
 *      Author: yura
 */

#ifndef EXT_CONFIGS_H_
#define EXT_CONFIGS_H_

#include <stdio.h>
#include <string.h>

#include "cs101_slave.h"
#include "hal_thread.h"
#include "libmodbus/modbus.h"

// define for set state on/off
#define SP_STATE_NO 0
#define SP_STATE_ON 1
#define SP_STATE_OFF 2

typedef enum
{
    cfg_ser_device = 0,
    cfg_port_baudrate,
    cfg_port_data_bit,
    cfg_port_parity,
    cfg_port_stop_bit,
    cfg_port_el_num
} cfg_device;

typedef enum
{
    cfg_mb_function = 0,
    cfg_mb_address,
    cfg_mb_size,
} cfg_mb_data;

typedef enum
{
    cfg_iec_function = 0,
    cfg_iec_ioa_addr,
    cfg_iec_size
} cfg_iec_data;

typedef enum
{
	cfg_btsw_dcba = 0,
	cfg_btsw_abcd,
	cfg_btsw_badc,
	cfg_btsw_cdab
} cfg_byte_swap;

typedef enum
{
	cfg_onoff_state = 0,
	cfg_onoff_value
} cfg_on_off;


typedef struct  {
	char*		func_str;
	TypeID		func_n;
} cfg_iec_func;

typedef enum  {
	cfg_prior_low =0,
	cfg_prior_hight,
} cfg_iec_prior;

typedef enum
{
	iec_priority = 0,
	iec_byteswap,
	iec_on_value,
	iec_off_value
} iec_set_params_flags;


typedef struct  {
	uint32_t				set_params;
	uint32_t				on_value;
	uint32_t				off_value;
	cfg_iec_prior		priority;
	cfg_byte_swap		byte_swap;
} iec_add_params;


typedef enum  {
	mem_init =0,
	mem_cur,
	mem_new,
	mem_chg,
	mem_err
} data_state;




typedef struct  {


	uint8_t				mem_size;
	data_state			mem_state;
	uint8_t				*mem_ptr;

}Value_mem;


typedef struct  {
	uint8_t				mb_func;
	uint16_t				mb_data_addr;
	uint16_t				mb_data_size;

	TypeID				iec_func;
	uint16_t				iec_ioa_addr;
	uint8_t				iec_size;

	iec_add_params		add_params;
//	cfg_byte_swap		byte_swap;
//	cfg_iec_prior		iec_priority;
//	uint8_t				on_off_flag;
//	uint16_t				on_off_data;

	uint8_t				mem_size;
	uint8_t				*mem_ptr;
	data_state			mem_state;
} Command_TypeDef;


typedef struct  {
	uint8_t		mb_slave_addr;
//	uint16_t		mb_poll_time;
	uint8_t		read_cmnds_num;
	Command_TypeDef		*read_cmnds;
	uint8_t		write_cmnds_num;
	Command_TypeDef		*write_cmnds;
} Modbus_Slave_TypeDef;


typedef struct  {
	char*		device;
	uint16_t		baud;
	uint8_t		data_bit;
	char		parity;
	uint8_t		stop_bit;
	uint16_t		recv_timeout;

	modbus_t	*mb_protocol_ptr;
	Thread		mb_thread;
	bool		mb_thread_run;
	bool		mb_thread_stop;

	Modbus_Slave_TypeDef		*mb_slave;
	uint8_t						num_slaves;
} Serial_Port_TypeDef;

typedef struct  {
	uint8_t						log_level;
	uint16_t						iec104_send_rate;
	Serial_Port_TypeDef			*serialport;
	uint8_t						num_ports;
} Transl_Config_TypeDef;





Transl_Config_TypeDef* read_config_file(const char *filename);


#endif /* EXT_CONFIGS_H_ */

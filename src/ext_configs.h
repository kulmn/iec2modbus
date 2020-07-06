/*
 * ext_configs.h
 *
 *  Created on: 23 мая 2020 г.
 *      Author: yura
 */

#ifndef EXT_CONFIGS_H_
#define EXT_CONFIGS_H_


#include <string.h>

#include "cs101_slave.h"
#include "hal_thread.h"
#include "memory.h"
#include "libmodbus/modbus.h"
#include "iec104_server.h"



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
    cfg_modbus_rtu_m = 0,
    cfg_modbus_rtu_s,
    cfg_iec_101,
    cfg_iec_103,
} cfg_ser_protocol;



typedef struct  {
	uint8_t				mb_func;
	uint16_t				mb_data_addr;
	uint16_t				mb_data_size;
	data_mem			*value;
}modbus_command;



typedef struct  {
	uint8_t					mb_slave_addr;
	uint8_t					mb_read_cmd_num;
	modbus_command		*mb_read_cmds;
	uint8_t					mb_write_cmd_num;
	modbus_command		*mb_write_cmds;
} Modbus_Slave_TypeDef;


typedef struct  {
	char*						device;
	uint16_t						baud;
	uint8_t						data_bit;
	char						parity;
	uint8_t						stop_bit;
	cfg_ser_protocol				protocol;
	uint16_t						recv_timeout;

	modbus_t					*mb_protocol_ptr;
	Thread						mb_thread;
	bool						mb_thread_run;
	bool						mb_thread_stop;

	Modbus_Slave_TypeDef		*mb_slave;
	uint8_t						num_slaves;
} Serial_Port_TypeDef;





typedef struct  {
	uint8_t						log_level;
//	uint16_t						iec104_send_rate;
	Serial_Port_TypeDef			*serialport;
	uint8_t						num_ports;

//	uint16_t						iec104_slave_num;
//	iec104_slave					*iec104_slave;

//	iec104_server				iec104_server;
} Transl_Config_TypeDef;





bool read_config_file(const char *filename,Transl_Config_TypeDef *config ,iec104_server *iec104_server);


#endif /* EXT_CONFIGS_H_ */

/*
 * ext_configs.h
 *
 *  Created on: 23 мая 2020 г.
 *      Author: yura
 */

#ifndef EXT_CONFIGS_H_
#define EXT_CONFIGS_H_


#include <string.h>
#include <inttypes.h>

#include "memory.h"
#include "modbus_m.h"
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
	SerialPort					serial_port;				// SerialPort (lib60870-C)
	cfg_ser_protocol				protocol;

//	Modbus_Master				mb_master;
	void*						protocol_ptr;

} Virt_Port;


typedef struct  {
	uint8_t						log_level;
	uint16_t						set_asdu_addr;
	Virt_Port					*virt_port;
	uint8_t						num_ports;
} Transl_Config_TypeDef;





bool read_config_file(const char *filename,Transl_Config_TypeDef *config ,iec104_server *iec104_server);


#endif /* EXT_CONFIGS_H_ */

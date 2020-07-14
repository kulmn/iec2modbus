/*
 * modbus_m.h
 *
 *  Created on: 30 мая 2020 г.
 *      Author: yura
 */

#ifndef MODBUS_M_H_
#define MODBUS_M_H_

#include <inttypes.h>

#include "libmodbus/modbus.h"
#include "memory.h"
#include "hal_serial.h"
#include "hal_thread.h"


struct sSerialPort {
    char interfaceName[100];
    int fd;
    int baudRate;
    uint8_t dataBits;
    char parity;
    uint8_t stopBits;
    uint64_t lastSentTime;
    struct timeval timeout;
    SerialPortError lastError;
};

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
	modbus_t					*mb_protocol_ptr;
	Thread						mb_thread;
	bool						mb_thread_run;
	bool						mb_thread_stop;
	uint8_t						num_slaves;
	Modbus_Slave_TypeDef		*mb_slave;
	uint16_t						recv_timeout;
}Modbus_Master;

Modbus_Master *Modbus_create(void);
int Modbus_Init(SerialPort port, Modbus_Master *master, bool debug);
void Modbus_Thread_Start(Modbus_Master *master);
void Modbus_Thread_Stop(Modbus_Master *master);
int modbus_read(modbus_t* mb_ptr, Modbus_Slave_TypeDef *slave);

#endif /* MODBUS_M_H_ */

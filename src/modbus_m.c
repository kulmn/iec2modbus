/*
 * modbus_m.c
 *
 *  Created on: 30 мая 2020 г.
 *      Author: yura
 */


#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include "modbus_m.h"

#include "libmodbus/modbus.h"
#include "slog.h"

#ifdef MOXA_UC8410
#include <moxadevice.h>
#endif


int modbus_write(modbus_t *mb_ptr, Modbus_Slave_TypeDef *slave)	//FIXME ALL
{
	int rc;

	rc = modbus_set_slave(mb_ptr, slave->mb_slave_addr );
	if (rc == -1)
	{
		fprintf(stderr, "Invalid slave ID\n" );
		return -1;
	}

	for (int i = 0; i < slave->write_cmnds_num; i++)
	{
		switch (slave->write_cmnds[i].mb_func)
		{
			case MODBUS_FC_WRITE_SINGLE_COIL: {
				if (slave->write_cmnds[i].mem_state == mem_new)
				{
					int status = (uint8_t) slave->write_cmnds[i].mem_ptr[0];
					rc = modbus_write_bit(mb_ptr, slave->write_cmnds[i].mb_data_addr, status );
					slave->write_cmnds[i].mem_state = mem_cur;
				}
			}break;
			case MODBUS_FC_WRITE_MULTIPLE_COILS:		// FIXME
			{
//				if (slave->write_cmnds[i].mem_state == mem_new)
//				{
//					rc = modbus_write_bits(mb_ptr, slave->write_cmnds[i].mb_data_addr, slave->write_cmnds[i].mb_data_size, slave->write_cmnds[i].mem_ptr );
//				}
			}break;
			case MODBUS_FC_WRITE_SINGLE_REGISTER:
			{
				if (slave->write_cmnds[i].mem_state == mem_new)
				{
					uint16_t data = (uint16_t) slave->write_cmnds[i].mem_ptr[0];
					rc = modbus_write_register(mb_ptr, slave->write_cmnds[i].mb_data_addr, data );
					slave->write_cmnds[i].mem_state = mem_cur;
				}
			}break;
			case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
			{
				if (slave->write_cmnds[i].mem_state == mem_new)
				{
					rc = modbus_write_registers(mb_ptr, slave->write_cmnds[i].mb_data_addr, slave->write_cmnds[i].mb_data_size, (const uint16_t*) slave->write_cmnds[i].mem_ptr );
					slave->write_cmnds[i].mem_state = mem_cur;
				}
			}break;
		}
	}
	return rc;
}


int modbus_read(modbus_t* mb_ptr, Modbus_Slave_TypeDef *slave)
{
	int rc;
	uint8_t *data_ptr = NULL;

	rc = modbus_set_slave(mb_ptr, slave->mb_slave_addr );
	if (rc == -1)
	{
		fprintf(stderr, "Invalid slave ID\n" );
		return -1;
	}

	for (int i = 0; i < slave->read_cmnds_num; i++)
	{
		data_ptr = (uint8_t*) malloc(slave->read_cmnds[i].mem_size );
		if (data_ptr == NULL)
		{
			slog_error("Unable to allocate %d bytes for modbus read data  ",slave->read_cmnds[i].mem_size);
			return -1;
		}
		switch (slave->read_cmnds[i].mb_func)
		{
			case MODBUS_FC_READ_COILS:
			{
				rc = modbus_read_bits(mb_ptr, slave->read_cmnds[i].mb_data_addr, slave->read_cmnds[i].mb_data_size, data_ptr );
			}break;
			case MODBUS_FC_READ_DISCRETE_INPUTS:
			{
				rc = modbus_read_input_bits(mb_ptr, slave->read_cmnds[i].mb_data_addr, slave->read_cmnds[i].mb_data_size, data_ptr );
			}break;
			case MODBUS_FC_READ_HOLDING_REGISTERS:
			{
				rc = modbus_read_registers(mb_ptr, slave->read_cmnds[i].mb_data_addr, slave->read_cmnds[i].mb_data_size,\
						(uint16_t*) data_ptr );
			}break;
			case MODBUS_FC_READ_INPUT_REGISTERS:
			{
				rc = modbus_read_input_registers(mb_ptr, slave->read_cmnds[i].mb_data_addr, slave->read_cmnds[i].mb_data_size,\
						(uint16_t*) data_ptr );
			}break;
			default:
			{
				// Error function not supported		//FIXME
			}
		}
		data_state	state;
		if (rc == -1) state = mem_err;
		else if (memcmp(data_ptr, slave->read_cmnds[i].mem_ptr, slave->read_cmnds[i].mem_size) == 0)
		{
			memcpy(slave->read_cmnds[i].mem_ptr, data_ptr, slave->read_cmnds[i].mem_size);
			state = mem_chg;
		}
		else
		{
			memcpy(slave->read_cmnds[i].mem_ptr, data_ptr, slave->read_cmnds[i].mem_size);
			state = mem_new;
		}
		slave->read_cmnds[i].mem_state = state;
		free(data_ptr);

	}

	return rc;
}


int Modbus_Init(Serial_Port_TypeDef *port, bool debug)
{
	modbus_t *ctx = NULL;
	ctx = modbus_new_rtu((const char*) port->device, port->baud, port->parity, port->data_bit, port->stop_bit );
	if (ctx == NULL)
	{
		slog_error("Unable to allocate libmodbus context" );
		modbus_free(ctx );
		return 1;
	}
	modbus_set_debug(ctx, debug );
//	modbus_set_error_recovery(ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL );
	modbus_set_error_recovery(ctx, MODBUS_ERROR_RECOVERY_NONE );
	modbus_set_byte_timeout(ctx, -1, 0 );			// disabled
	modbus_set_response_timeout(ctx, 0, (port->recv_timeout)*1000 );

	port->mb_thread = NULL;
	port->mb_protocol_ptr = NULL;
	port->mb_thread_run = false;

	if (modbus_connect(ctx ) == -1)
	{
		slog_warn("Connection failed: %s ", modbus_strerror(errno ) );
		modbus_free(ctx );
		return 1;
	}
#ifdef MOXA_UC8410
	if (modbus_rtu_set_serial_mode(ctx,  RS485_2WIRE_MODE) == -1)
	{
		slog_warn("Set RS485 port mode failed: %s ", modbus_strerror(errno ) );
		modbus_free(ctx );
		return 1;
	}
#endif
	port->mb_protocol_ptr = ctx;
	port->mb_thread_run = false;
	return 0;
}


static void* ModbusThread(void *parameter)
{
	Serial_Port_TypeDef *port = (Serial_Port_TypeDef*) parameter;

	while (port->mb_thread_stop == false)
	{
		for (int j = 0; j < port->num_slaves; j++)
		{
			if (modbus_write(port->mb_protocol_ptr, &port->mb_slave[j] ) == -1)
			{
				// Error			FIXME
			}
			Thread_sleep(30);
			if (modbus_read(port->mb_protocol_ptr, &port->mb_slave[j] ) == -1)
			{
				// Error			FIXME
			}
			Thread_sleep(30);

		}

		Thread_sleep(1000 );
	}

	port->mb_thread_run = false;
	port->mb_thread_stop = false;

//	exit_function:
	return NULL;
}


void Modbus_Thread_Start(Serial_Port_TypeDef *port)
{
	if (port->mb_thread_run == false)
	{
		port->mb_thread_run = true;
		port->mb_thread_stop = false;
		port->mb_thread = Thread_create(ModbusThread, (void*) port, false );
		Thread_start(port->mb_thread );
	}
}

void Modbus_Thread_Stop(Serial_Port_TypeDef *port)
{
	if (port->mb_thread_run)
	{
		port->mb_thread_stop = true;
		while (port->mb_thread_run)
			Thread_sleep(1 );
	}
	if (port->mb_thread)
	{
		Thread_destroy(port->mb_thread );
	}
	port->mb_thread = NULL;

	if (port->mb_protocol_ptr != NULL)
	{
		modbus_close(port->mb_protocol_ptr );
		modbus_free(port->mb_protocol_ptr );
	}

}

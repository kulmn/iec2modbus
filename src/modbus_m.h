/*
 * modbus_m.h
 *
 *  Created on: 30 мая 2020 г.
 *      Author: yura
 */

#ifndef MODBUS_M_H_
#define MODBUS_M_H_

#include "ext_configs.h"

int Modbus_Init(Serial_Port_TypeDef *port, bool debug);
void Modbus_Thread_Start(Serial_Port_TypeDef *port);
void Modbus_Thread_Stop(Serial_Port_TypeDef *port);
int modbus_read(modbus_t* mb_ptr, Modbus_Slave_TypeDef *slave);

#endif /* MODBUS_M_H_ */

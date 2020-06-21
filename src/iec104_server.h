/*
 * iec104_server.h
 *
 *  Created on: 25 мая 2020 г.
 *      Author: kulish_y
 */

#ifndef IEC104_SERVER_H_
#define IEC104_SERVER_H_

#include "ext_configs.h"
#include <errno.h>

#include "cs104_slave.h"
#include "hal_time.h"
#include "moxa_dev.h"

#define MAX_CMD_CNT		4


CS104_Slave iec104_server_init( Transl_Config_TypeDef *config, bool debug );
int iec104_send_changed_data(CS104_Slave slave, Transl_Config_TypeDef *config, cfg_iec_prior priority);
void iec104_server_start(void);

#endif /* IEC104_SERVER_H_ */

/*
 * main.h
 *
 *  Created on: 22 мая 2020 г.
 *      Author: kulish_y
 */

#ifndef MAIN_H_
#define MAIN_H_

#define VERSION_MAJOR		0
#define VERSION_MINOR	4


#ifdef MOXA_UC8410
#include <moxadevice.h>
#endif


#include <features.h>

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <inttypes.h>
# include <stdint.h>



#include "libmodbus/modbus.h"
#include "ext_configs.h"
#include "iec104_server.h"
#include "modbus_m.h"
#include "memory.h"
#include "slog.h"

#include "moxa_dev.h"















#endif /* MAIN_H_ */

/*
 * memory.h
 *
 *  Created on: 5 июл. 2020 г.
 *      Author: yura
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdio.h>
#include <pthread.h>
//#include "ext_configs.h"

typedef enum  {
	data_bool =0,
	data_uint16,
	data_uint32,
} data_type;

typedef enum  {
	mem_init =0,
	mem_cur,
	mem_new,
	mem_chg,
	mem_err
} data_state;

typedef struct  {
	uint8_t				mem_size;
	data_type			mem_type;
	data_state			mem_state;
	uint8_t				*mem_ptr;
	pthread_mutex_t	lock;
}data_mem;







#endif /* MEMORY_H_ */

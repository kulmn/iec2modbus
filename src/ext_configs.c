/*
 * ext_configs.c
 *
 *  Created on: 23 мая 2020 г.
 *      Author: yura
 */

#include "ext_configs.h"

#include "../include/json-c/json.h"
#include "slog.h"



// Just a utility function.
 void print_json_object(struct json_object *jobj, const char *msg)
{
	printf("\n%s: \n", msg );
	printf("---\n%s\n---\n", json_object_to_json_string(jobj ) );
}
 bool allocate_all_cmd_memory(modbus_command *cmd)
 {
 	uint8_t size_in_bytes = 0;

 	switch (cmd->mb_func)
 	{
 		case MODBUS_FC_READ_COILS:
  		case MODBUS_FC_WRITE_MULTIPLE_COILS:
 		case MODBUS_FC_READ_DISCRETE_INPUTS:
 		{
 			size_in_bytes = (cmd->mb_data_size >> 3) + 1;
 		}break;
 		case MODBUS_FC_WRITE_SINGLE_COIL:
 		{
 		 	size_in_bytes =  sizeof(uint8_t);
 		 }break;
 		case MODBUS_FC_READ_HOLDING_REGISTERS:
 		case MODBUS_FC_READ_INPUT_REGISTERS:
 		case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
 		{
 			size_in_bytes = (cmd->mb_data_size * sizeof(uint16_t));
 		}break;
 		case MODBUS_FC_WRITE_SINGLE_REGISTER:
 		{
 			size_in_bytes = sizeof(uint16_t);
 		}break;
 	}
 	if (size_in_bytes == 0)
 	{
 		slog_error("Data size for command = 0, mb_fn=%d ",  cmd->mb_func );
 		return false;
 	}
 	cmd->value->mem_ptr = (uint8_t*) malloc(size_in_bytes );

 	if (cmd->value->mem_ptr == NULL)
 	{
 		slog_error("Unable to allocate %d bytes for read_command, mb_fn=%d ", size_in_bytes, cmd->mb_func );
 		return false;
 	}
 	memset(cmd->value->mem_ptr, 0, size_in_bytes );

 	cmd->value->mem_size = size_in_bytes;
 	cmd->value->mem_state = mem_init;
 	return true;
 }

bool allocate_slave_cmd_memory(Modbus_Slave_TypeDef *mb_slave, iec104_server *iec104_server, uint8_t iec104_slave_cnt)
{
	for (int x = 0; x < mb_slave->mb_read_cmd_num; x++) // Modbus slave read commands num
	{
		data_mem *ptr = malloc(sizeof(data_mem) );//FIXME add malloc error
		if (pthread_mutex_init(&ptr->lock, NULL ) != 0)
		{
			slog_error("mutex init failed" );
			return false;					// FIXME
		}
		mb_slave->mb_read_cmds[x].value = ptr;
		iec104_server->iec104_slave[iec104_slave_cnt].iec104_read_cmds[x].value = ptr;
		allocate_all_cmd_memory(&mb_slave->mb_read_cmds[x] );
	}

	for (int x = 0; x < mb_slave->mb_write_cmd_num; x++) // Modbus slave write commands num
	{
		data_mem *ptr = malloc(sizeof(data_mem) );
		if (pthread_mutex_init(&ptr->lock, NULL ) != 0)
		{
			slog_error("mutex init failed" );
			return false;					// FIXME
		}
		mb_slave->mb_write_cmds[x].value = ptr;
		iec104_server->iec104_slave[iec104_slave_cnt].iec104_write_cmds[x].value = ptr;
		allocate_all_cmd_memory(&mb_slave->mb_write_cmds[x] );
	}
	iec104_slave_cnt++;

	return true;
}
/*
bool allocate_cmd_memory(Modbus_Master *mb_master, iec104_server *iec104_server, uint8_t iec104_slave_cnt)
{
		for (int j = 0; j < mb_master->num_slaves; j++)// Modbus slaves num
		{
			for (int x = 0; x < mb_master->mb_slave[j].mb_read_cmd_num; x++) // Modbus slave read commands num
			{
				data_mem *ptr = malloc(sizeof(data_mem) );			//FIXME add malloc error
				 if (pthread_mutex_init(&ptr->lock, NULL) != 0)
				 {
					 slog_error("mutex init failed");
					 return false;					// FIXME
				 }
				mb_master->mb_slave[j].mb_read_cmds[x].value = ptr;
				iec104_server->iec104_slave[iec104_slave_cnt].iec104_read_cmds[x].value =ptr;
				allocate_all_cmd_memory(&mb_master->mb_slave[j].mb_read_cmds[x] );
			}

			for (int x = 0; x < mb_master->mb_slave[j].mb_write_cmd_num; x++) // Modbus slave write commands num
			{
				data_mem *ptr = malloc(sizeof(data_mem) );
				 if (pthread_mutex_init(&ptr->lock, NULL) != 0)
				{
					 slog_error("mutex init failed");
					 return false;					// FIXME
				}
				 mb_master->mb_slave[j].mb_write_cmds[x].value = ptr;
				iec104_server->iec104_slave[iec104_slave_cnt].iec104_write_cmds[x].value =ptr;
				allocate_all_cmd_memory(&mb_master->mb_slave[j].mb_write_cmds[x] );
			}
			iec104_slave_cnt++;
		}

	return true;
}
*/
bool read_json_file(const char *filename, struct json_object **parsed_json)
{
	FILE *fp = NULL;
	uint8_t *buffer = NULL;
	long f_size;

	slog_debug( "fopen file: %s", filename);
	fp = fopen(filename, "r" );
	if (fp == NULL)
	{
		slog_error( "failed to fopen %s", filename);
		return false;
	}
	if (fseek(fp, 0, SEEK_END ) == -1)
	{
		slog_error( "ailed to fseek %s", filename);
		return false;
	}
	f_size = ftell(fp );
	if (f_size == (long) -1)
	{
		slog_error( "failed to ftell %s", filename);
		return false;
	}
	if (fseek(fp, 0, SEEK_SET ) == -1)
	{
		slog_error( "failed to fseek %s", filename);
		return false;
	}

	slog_debug( "Allocate memory for file: %s", filename);
	buffer = malloc(f_size + 1 );
	if (buffer == NULL)
	{
		slog_error( "failed to allocate memory for file %s", filename);
		return false;
	}

	buffer[0]=0;

	fread(buffer, 1, f_size, fp );

	*parsed_json = json_tokener_parse((const char *)buffer );				// parse read file
	if(*parsed_json == NULL) return false;
	free(buffer );
	if (fclose(fp ) != 0)
	{
		slog_error( "failed to fclose %s", filename);
		return false;
	}
	return true;
}


int parse_iec_add_params(struct json_object *add_parm_json, iec104_command *cmd )
{
	struct json_object *tmp_json=NULL;
	const char *str;

	int arr_len = json_object_array_length(add_parm_json);
	for(int i = 0; i < arr_len; i++)
	{
		tmp_json = json_object_array_get_idx(add_parm_json, i);
		str = json_object_get_string(tmp_json);
		char *param = strtok( (char*) str, "=");
		if (param != NULL)
		{
			char *value = strtok(NULL, "=");

			if ( !strcmp(param, "priority") )
			{
				if ( !strcmp(value, "hight") )  cmd->add_params.priority = cfg_prior_hight;
				cmd->add_params.set_params |= (1 << iec_priority);
			}
			else if ( !strcmp(param, "byteswap") )
			{
				if ( !strcmp(value, "abcd") ) cmd->add_params.byte_swap = cfg_btsw_abcd;
				else if ( !strcmp(value, "badc") ) cmd->add_params.byte_swap = cfg_btsw_badc;
				else if ( !strcmp(value, "cdab") ) cmd->add_params.byte_swap = cfg_btsw_cdab;
				else {} //use default
				cmd->add_params.set_params |= (1 << iec_byteswap);
			}
			else if ( !strcmp(param, "bitmask") )
			{
				cmd->add_params.bitmask = strtol(value, NULL, 0);
				cmd->add_params.set_params |= (1 << iec_bitmask);
			}
			else if ( !strcmp(param, "ON") )
			{
				cmd->add_params.on_value  = strtol(value, NULL, 0);
				cmd->add_params.set_params |= (1 << iec_on_value);
			}
			else if ( !strcmp(param, "OFF") )
			{
				cmd->add_params.off_value  = strtol(value, NULL, 0);
				cmd->add_params.set_params |= (1 << iec_off_value);
			}
			else
			{
				slog_error( "Wrong modbus read function: '%s' .", str);
				return false;
			}
		}
	}
	return true;
}


bool parse_modbus_read_cmd(struct json_object *cur_cmd, modbus_command *read_cmd )
{
	const char *str;
	struct json_object *tmp_json=NULL, *mb_data_json=NULL;

	json_object_object_get_ex(cur_cmd, "modbus_data", &mb_data_json );
	// parse modbus function code
	tmp_json = json_object_array_get_idx(mb_data_json, cfg_mb_function);
	str = json_object_get_string(tmp_json);

	if ( !strcmp(str, "04_read_input") )  read_cmd->mb_func = MODBUS_FC_READ_INPUT_REGISTERS;
	else if ( !strcmp(str, "03_read_holding") ) read_cmd->mb_func = MODBUS_FC_READ_HOLDING_REGISTERS;
	else if ( !strcmp(str, "02_read_discrete") ) read_cmd->mb_func = MODBUS_FC_READ_DISCRETE_INPUTS;
	else if ( !strcmp(str, "01_read_coils") ) read_cmd->mb_func = MODBUS_FC_READ_COILS;
	else
	{
		slog_error( "Wrong modbus read function: '%s' .", str);
		return false;
	}
	// parse modbus data address
	tmp_json = json_object_array_get_idx(mb_data_json, cfg_mb_address);
	str = json_object_get_string(tmp_json);
	read_cmd->mb_data_addr = strtol(str, NULL, 0);
	// parse modbus data size
	tmp_json = json_object_array_get_idx(mb_data_json, cfg_mb_size);
	read_cmd->mb_data_size = json_object_get_int(tmp_json);

	return true;
}

bool parse_iec104_read_cmd(struct json_object *cur_cmd, iec104_command *read_cmd )
{
	const char *str;
	struct json_object *tmp_json=NULL, *iec_data_json=NULL;

	json_object_object_get_ex(cur_cmd, "iec104_data", &iec_data_json );
	// parse iec104 function code
	tmp_json = json_object_array_get_idx(iec_data_json, cfg_iec_function);
	str = json_object_get_string(tmp_json);

	read_cmd->iec_func = String_to_TypeID(str);
	if (read_cmd->iec_func == 0)
	{
		slog_error( "Wrong iec104 read function: '%s' .", str);
		return false;
	}
	// parse iec104 ioa address
	tmp_json = json_object_array_get_idx(iec_data_json, cfg_iec_ioa_addr);
	str = json_object_get_string(tmp_json);
	read_cmd->iec_ioa_addr = strtol(str, NULL, 0);

	// parse iec104  size
	tmp_json = json_object_array_get_idx(iec_data_json, cfg_iec_size);
	read_cmd->iec_size = json_object_get_int(tmp_json);

	// Additional parameters
	struct json_object *add_parm_json=NULL;
	read_cmd->add_params.priority = cfg_prior_low;
	read_cmd->add_params.byte_swap = cfg_btsw_dcba;
	read_cmd->add_params.set_params = 0;
	if (json_object_object_get_ex(cur_cmd, "add_param", &add_parm_json ))
	{
		parse_iec_add_params( add_parm_json , read_cmd );
	}

	return true;
}


int parse_modbus_write_cmd(struct json_object *cur_cmd, modbus_command *write_cmd )
{
	const char *str;
	struct json_object *tmp_json=NULL, *mb_data_json=NULL, *iec_data_json=NULL;

	json_object_object_get_ex(cur_cmd, "modbus_data", &mb_data_json );

	// parse function code
	tmp_json = json_object_array_get_idx(mb_data_json, cfg_mb_function);
	str = json_object_get_string(tmp_json);
	if ( !strcmp(str, "06_write_singl_holding") ) write_cmd->mb_func = MODBUS_FC_WRITE_SINGLE_REGISTER;
	else if ( !strcmp(str, "16_write_multiple_holding") ) write_cmd->mb_func = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
	else if ( !strcmp(str, "05_write_singl_coil") ) write_cmd->mb_func = MODBUS_FC_WRITE_SINGLE_COIL;
//	else if ( !strcmp(str, "write_multiple_coil") ) write_cmd->mb_func = MODBUS_FC_WRITE_MULTIPLE_COILS;
	else
	{
		slog_error( "Wrong modbus write function: '%s' .", str);
		return false;
	}

	// parse modbus data address
	tmp_json = json_object_array_get_idx(mb_data_json, cfg_mb_address);
	str = json_object_get_string(tmp_json);
	write_cmd->mb_data_addr = strtol(str, NULL, 0);

	json_object_object_get_ex(cur_cmd, "iec104_data", &iec_data_json );
	// parse iec104 function code
	tmp_json = json_object_array_get_idx(iec_data_json, cfg_iec_function);
	str = json_object_get_string(tmp_json);
	// parse modbus data size
	tmp_json = json_object_array_get_idx(mb_data_json, cfg_mb_size);
	write_cmd->mb_data_size = json_object_get_int(tmp_json);

	return true;
}

int parse_iec104_write_cmd(struct json_object *cur_cmd, iec104_command *write_cmd )
{
	const char *str;
	struct json_object *tmp_json=NULL, *iec_data_json=NULL;

	json_object_object_get_ex(cur_cmd, "iec104_data", &iec_data_json );
	// parse iec104 function code
	tmp_json = json_object_array_get_idx(iec_data_json, cfg_iec_function);
	str = json_object_get_string(tmp_json);

	write_cmd->iec_func = String_to_TypeID(str);
	if (write_cmd->iec_func == 0)
	{
		slog_error( "Wrong iec104 write function: '%s' .", str);
		return false;
	}
	// parse iec104 ioa address
	tmp_json = json_object_array_get_idx(iec_data_json, cfg_iec_ioa_addr);
	str = json_object_get_string(tmp_json);
	write_cmd->iec_ioa_addr = strtol(str, NULL, 0);

	// Additional parameters
	write_cmd->add_params.set_params = 0;
	struct json_object *add_parm_json=NULL;
	if (json_object_object_get_ex(cur_cmd, "add_param", &add_parm_json ))
	{
		parse_iec_add_params( add_parm_json , write_cmd );
	}

	return true;
}

bool parse_slave_iec104_config(struct json_object *parsed_json, iec104_slave *iec_slave )
{
	struct json_object *read_cmd_json=NULL, *write_cmd_json=NULL, *cur_cmd=NULL;
	iec104_command *cmd_ptr = NULL;

	json_object_object_get_ex(parsed_json, "read_commands", &read_cmd_json );
	int read_cmd_num = json_object_array_length(read_cmd_json );

	iec_slave->iec104_read_cmd_num =  0;
	iec_slave->iec104_read_cmds = NULL;
	for (uint8_t i = 0; i < read_cmd_num; i++)
	{
		cmd_ptr = iec104_add_slave_rd_cmd( iec_slave );
		cur_cmd = json_object_array_get_idx(read_cmd_json, i );
		if ( ! parse_iec104_read_cmd( cur_cmd, cmd_ptr ))
		{
			slog_error( "Parsing read commands failed.");
			return false;
		}
	}

	json_object_object_get_ex(parsed_json, "write_commands", &write_cmd_json );
	int write_cmd_num = json_object_array_length(write_cmd_json );

	//iec_slave->iec104_write_cmd_num = write_cmd_num;
	//iec_slave->iec104_write_cmds = (iec104_command*) malloc(iec_slave->iec104_write_cmd_num * sizeof(iec104_command) );

	iec_slave->iec104_write_cmd_num = 0;
	iec_slave->iec104_write_cmds = NULL;
	for (int i = 0; i < write_cmd_num; i++)
	{
		cmd_ptr = iec104_add_slave_wr_cmd( iec_slave );
		cur_cmd = json_object_array_get_idx(write_cmd_json, i );
		if ( ! parse_iec104_write_cmd( cur_cmd, cmd_ptr ))
		{
			slog_error( "Parsing write commands failed.");
			return false;
		}
	}
	return true;
}

bool parse_slave_modbus_config(struct json_object *parsed_json,Modbus_Slave_TypeDef *mb_slave )
{
	struct json_object *read_cmd_json=NULL, *write_cmd_json=NULL, *cur_cmd=NULL;
	modbus_command *cmd_ptr = NULL;

	json_object_object_get_ex(parsed_json, "read_commands", &read_cmd_json );
	int read_cmd_num = json_object_array_length(read_cmd_json );

	mb_slave->mb_read_cmd_num =  0;
	mb_slave->mb_read_cmds = NULL;
	for (uint8_t i = 0; i < read_cmd_num; i++)
	{
		cmd_ptr = mb_add_slave_rd_cmd( mb_slave );
		cur_cmd = json_object_array_get_idx(read_cmd_json, i );
		if ( ! parse_modbus_read_cmd( cur_cmd, cmd_ptr ))
		{
			slog_error( "Parsing read commands failed.");
			return false;
		}
	}


	json_object_object_get_ex(parsed_json, "write_commands", &write_cmd_json );
	int write_cmd_num = json_object_array_length(write_cmd_json );

	mb_slave->mb_write_cmd_num =  0;
	mb_slave->mb_write_cmds = NULL;
	for (uint8_t i = 0; i < write_cmd_num; i++)
	{
		cmd_ptr = mb_add_slave_wr_cmd( mb_slave );
		cur_cmd = json_object_array_get_idx(write_cmd_json, i );
		if ( ! parse_modbus_write_cmd( cur_cmd, cmd_ptr ))
		{
			slog_error( "Parsing read commands failed.");
			return false;
		}
	}

	return true;
}

/******************* Set same ASDU address for all modbus devices *********************************************************/
void recalculate_iec104_addr(iec104_server *iec104_server, uint16_t set_asdu)
{
	for (int i = 0; i < iec104_server->iec104_slave_num ; i++)
	{
		for (int j = 0; j < iec104_server->iec104_slave[i].iec104_read_cmd_num; j++)
		{
			iec104_server->iec104_slave[i].iec104_read_cmds[j].iec_ioa_addr |= (iec104_server->iec104_slave[i].iec_asdu_addr << 12);
			slog_debug( "slave: %d , read_cmd: %d, addr: %d ", i, j, iec104_server->iec104_slave[i].iec104_read_cmds[j].iec_ioa_addr);
		}

		for (int j = 0; j < iec104_server->iec104_slave[i].iec104_write_cmd_num; j++)
			iec104_server->iec104_slave[i].iec104_write_cmds[j].iec_ioa_addr |= (iec104_server->iec104_slave[i].iec_asdu_addr << 12);

		iec104_server->iec104_slave[i].iec_asdu_addr = set_asdu;
		slog_debug( "Set ASDU address : %d", set_asdu);
	}
}

bool read_config_file(const char *filename,Transl_Config_TypeDef *config ,iec104_server *iec104_server)
{
	struct json_object *parsed_json=NULL;
	struct json_object *ports=NULL, *cur_port=NULL, *port_device=NULL , *mb_slaves=NULL, *cur_slave=NULL;

	struct json_object *tmp_json=NULL;
	const char *str;
	size_t tmp;


	slog_info( "Read main config file: '%s' ", filename);
	if (!read_json_file(filename,&parsed_json))
	{
		slog_error( "Read config file: '%s' failed.", filename);
		return false;
	}

	// parse log_level
	json_object_object_get_ex(parsed_json, "log_level", &tmp_json );
	str = json_object_get_string(tmp_json );
	if ( !strcmp(str, "debug") )  config->log_level = SL_DEBUG;
	else if ( !strcmp(str, "info") ) config->log_level = SL_INFO;
	else if ( !strcmp(str, "warning") ) config->log_level = SL_WARN;
	else if ( !strcmp(str, "error") ) config->log_level = SL_ERROR;
	else
	{
		config->log_level = SL_INFO;
		slog_warn("Wrong log_level: %s, use default.", str );
	}

	// parse iec104 ASDU address
	config->set_asdu_addr = 0;
	if (json_object_object_get_ex(parsed_json, "asdu_address", &tmp_json ))
		config->set_asdu_addr = json_object_get_int(tmp_json);;

	// parse iec104_send_rate
	json_object_object_get_ex(parsed_json, "iec104_send_rate_s", &tmp_json );
	iec104_server->iec104_send_rate = json_object_get_int(tmp_json);

	json_object_object_get_ex(parsed_json, "serial_ports", &ports );
	config->num_ports = json_object_array_length(ports );
	config->virt_port = (Virt_Port*) malloc(config->num_ports * sizeof(Virt_Port) );


	for (int i = 0; i < config->num_ports; i++)
	{
		cur_port = json_object_array_get_idx(ports, i );

		// parse serial port param
		json_object_object_get_ex(cur_port, "device", &port_device );
		tmp = json_object_array_length(port_device );
		if (tmp != cfg_port_el_num)
		{
			slog_error( "Wrong number of elements in: 'serial_ports:%d\\device' section", i );
			return false;
		}

		// linux device string
		tmp_json = json_object_array_get_idx(port_device, cfg_ser_device);
		const char* interfaceName = json_object_get_string(tmp_json );
		if (strlen(str ) == 0)
		{
			slog_warn("Serial port %d device not specified. for example '/dev/ttyS0' ", i );
//			return NULL;	FIXME
		}
		// port baudrate
		tmp_json = json_object_array_get_idx(port_device, cfg_port_baudrate);
		int baudRate = json_object_get_int(tmp_json);
		// port data bit
		tmp_json = json_object_array_get_idx(port_device, cfg_port_data_bit);
		uint8_t dataBits = json_object_get_int(tmp_json);
		// port parity
		tmp_json = json_object_array_get_idx(port_device, cfg_port_parity);
		str = json_object_get_string(tmp_json);
		char parity = (char) *str;
		// port stop bit
		tmp_json = json_object_array_get_idx(port_device, cfg_port_stop_bit);
		uint8_t stopBits = json_object_get_int(tmp_json);
		config->virt_port[i].serial_port = SerialPort_create(interfaceName, baudRate, dataBits, parity, stopBits);

		// parse protocol
		json_object_object_get_ex(cur_port, "Protocol", &tmp_json );
		str = json_object_get_string(tmp_json);
		if ( !strcmp(str, "modbus_rtu_master") )
		{
			config->virt_port[i].protocol = cfg_modbus_rtu_m;
			config->virt_port[i].protocol_ptr = (Modbus_Master *) Modbus_create();
		}
		else if ( !strcmp(str, "modbus_rtu_slave") ) config->virt_port[i].protocol = cfg_modbus_rtu_s;
		else if ( !strcmp(str, "iec_60870-5-101") ) config->virt_port[i].protocol = cfg_iec_101;
		else if ( !strcmp(str, "iec_60870-5-103") ) config->virt_port[i].protocol = cfg_iec_103;
		else
		{
			slog_error( "Wrong serial protocol: '%s' .", str);
			return false;
		}
		Modbus_Master * mb_master = (Modbus_Master*) config->virt_port[i].protocol_ptr;

		// parse modbus answer timeout
		json_object_object_get_ex(cur_port, "answer_timeout_ms", &tmp_json );
		mb_master->recv_timeout = json_object_get_int(tmp_json);

		// parse modbus slaves param
		json_object_object_get_ex(cur_port, "slave", &mb_slaves );
		mb_master->num_slaves= json_object_array_length(mb_slaves );
		mb_master->mb_slave =
				(Modbus_Slave_TypeDef*) malloc(mb_master->num_slaves * sizeof(Modbus_Slave_TypeDef) );
	}


	uint8_t iec104_slave_cnt = 0;
	for (int i = 0; i < config->num_ports; i++)
	{
		Modbus_Master * mb_master = (Modbus_Master*) config->virt_port[i].protocol_ptr;

		cur_port = json_object_array_get_idx(ports, i );

		// parse modbus slaves param
		json_object_object_get_ex(cur_port, "slave", &mb_slaves );

		for (int j = 0; j < mb_master->num_slaves; j++)
		{
			cur_slave = json_object_array_get_idx(mb_slaves, j );
			// slave addr
			json_object_object_get_ex(cur_slave, "slave_address", &tmp_json );
			int slave_addr = json_object_get_int(tmp_json );
			mb_master->mb_slave[j].mb_slave_addr = slave_addr;
			iec104_add_slave( iec104_server,  slave_addr );
			// slave config file
			json_object_object_get_ex(cur_slave, "slave_config_file", &tmp_json );
			str = json_object_get_string(tmp_json );

			// read and parse slave config file
			struct json_object *parsed_slave_json=NULL;
			if (!read_json_file(str,&parsed_slave_json) )	return false;

			if (!parse_slave_iec104_config(parsed_slave_json, &iec104_server->iec104_slave[iec104_slave_cnt] ))
			{
				slog_error(" Loading : 'serial_ports:%d\\slave:%d\\slave_config_file: %s'  failed. ", i, j, str );
				return false;
			}
			if (!parse_slave_modbus_config(parsed_slave_json, &mb_master->mb_slave[j] ))
			{
				slog_error(" Loading : 'serial_ports:%d\\slave:%d\\slave_config_file: %s'  failed. ", i, j, str );
				return false;
			}
			json_object_put(parsed_slave_json);	// free json
			// allocate data memory for commands
			allocate_slave_cmd_memory(&mb_master->mb_slave[j], iec104_server, iec104_slave_cnt);
			iec104_slave_cnt++;
		}
	}
	json_object_put(parsed_json);	// free

	if (config->set_asdu_addr != 0)			// Set same ASDU address for all modbus devices
		recalculate_iec104_addr(iec104_server, config->set_asdu_addr);


	slog_info( "Config file: '%s' successful read", filename);
	return true;
}



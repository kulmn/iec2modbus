/*
 * ext_configs.c
 *
 *  Created on: 23 мая 2020 г.
 *      Author: yura
 */

#include "ext_configs.h"

#include "../include/json-c/json.h"
#include "slog.h"


//#include <errno.h>


const cfg_iec_func iec_read_fn_str[] = {
{"M_SP_NA_1", M_SP_NA_1}, {"M_SP_TA_1", M_SP_TA_1}, {"M_DP_NA_1", M_DP_NA_1}, {"M_DP_TA_1", M_DP_TA_1}, {"M_ST_NA_1", M_ST_NA_1},
{"M_ST_TA_1", M_ST_TA_1}, {"M_BO_NA_1", M_BO_NA_1}, {"M_BO_TA_1", M_BO_TA_1}, {"M_ME_NA_1", M_ME_NA_1}, {"M_ME_TA_1", M_ME_TA_1},
{"M_ME_NB_1", M_ME_NB_1}, {"M_ME_TB_1", M_ME_TB_1}, {"M_ME_NC_1", M_ME_NC_1}, {"M_ME_TC_1", M_ME_TC_1}, {"M_IT_NA_1", M_IT_NA_1},
{"M_IT_TA_1", M_IT_TA_1}, {"M_EP_TA_1", M_EP_TA_1}, {"M_EP_TB_1", M_EP_TB_1}, {"M_EP_TC_1", M_EP_TC_1}, {"M_PS_NA_1", M_PS_NA_1},
{"M_ME_ND_1", M_ME_ND_1}, {"M_SP_TB_1", M_SP_TB_1}, {"M_DP_TB_1", M_DP_TB_1}, {"M_ST_TB_1", M_ST_TB_1}, {"M_BO_TB_1", M_BO_TB_1},
{"M_ME_TD_1", M_ME_TD_1}, {"M_ME_TE_1", M_ME_TE_1}, {"M_ME_TF_1", M_ME_TF_1}, {"M_IT_TB_1", M_IT_TB_1}, {"M_EP_TD_1", M_EP_TD_1},
{"M_EP_TE_1", M_EP_TE_1}, {"M_EP_TF_1", M_EP_TF_1},
};

const cfg_iec_func iec_write_fn_str[] = {
{"C_SC_NA_1", C_SC_NA_1}, {"C_DC_NA_1", C_DC_NA_1}, {"C_RC_NA_1", C_RC_NA_1}, {"C_SE_NA_1", C_SE_NA_1}, {"C_SE_NB_1", C_SE_NB_1},
{"C_SE_NC_1", C_SE_NC_1}, {"C_BO_NA_1", C_BO_NA_1}, {"C_SC_TA_1", C_SC_TA_1}, {"C_DC_TA_1", C_DC_TA_1}, {"C_RC_TA_1", C_RC_TA_1},
{"C_SE_TA_1", C_SE_TA_1}, {"C_SE_TB_1", C_SE_TB_1}, {"C_SE_TC_1", C_SE_TC_1}, {"C_BO_TA_1", C_BO_TA_1}, {"C_IC_NA_1", C_IC_NA_1},
{"C_CI_NA_1", C_CI_NA_1}, {"C_RD_NA_1", C_RD_NA_1}, {"C_CS_NA_1", C_CS_NA_1}, {"C_TS_NA_1", C_TS_NA_1}, {"C_RP_NA_1", C_RP_NA_1},
{"C_CD_NA_1", C_CD_NA_1}, {"C_TS_TA_1", C_TS_TA_1},
};



// Just a utility function.
 void print_json_object(struct json_object *jobj, const char *msg)
{
	printf("\n%s: \n", msg );
	printf("---\n%s\n---\n", json_object_to_json_string(jobj ) );
}



 bool allocate_read_cmd_memory(modbus_command *cmd)
{
	uint8_t size_in_bytes = 0;

	switch (cmd->mb_func)
	{
		case MODBUS_FC_READ_COILS:
		case MODBUS_FC_READ_DISCRETE_INPUTS: {
			size_in_bytes = (cmd->mb_data_size >> 3) + 1;
		}break;
		case MODBUS_FC_READ_HOLDING_REGISTERS:
		case MODBUS_FC_READ_INPUT_REGISTERS: {
			size_in_bytes = (cmd->mb_data_size * sizeof(uint16_t));
		}break;
	}
	if (size_in_bytes == 0)
	{
		slog_error("Data size for read_command = 0, mb_fn=%d ",  cmd->mb_func );
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

 bool allocate_write_cmd_memory(iec104_command *cmd)
 {
		uint8_t size_in_bytes = 0;
		switch (cmd->iec_func)
		{
			case C_SC_NA_1:			// Single command
//					case C_SC_TA_1:			// Single command with CP56Time2a
			case C_DC_NA_1: 			// Double command
//					case C_DC_TA_1: 			// Double command with CP56Time2a
//					case C_RC_NA_1:			// Regulating step command
//					case C_RC_TA_1:			// Regulating step command with CP56Time2a
			case C_SE_NA_1: 			// Setpoint command, normalized value
//					case C_SE_TA_1: 			// Setpoint command, normalized value with CP56Time2a
			case C_SE_NB_1: 			// Setpoint command, scaled value
//					case C_SE_TB_1: 			// Setpoint command, scaled value with CP56Time2a
			{
				size_in_bytes = sizeof(uint16_t);
				cmd->value->mem_type = data_uint16;
			}break;
			case C_SE_NC_1: // Setpoint command, short floating point value
//					case C_SE_TC_1: // Setpoint command, short floating point value with CP56Time2a
			case C_BO_NA_1: // Bit string 32 bit
//					case C_BO_TA_1: // Bit string 32 bit with CP56Time2a
			{
				size_in_bytes = sizeof(uint32_t);
				cmd->value->mem_type = data_uint32;
			}break;
			default:
			{
				slog_error("Unsupported ASDU type id #:%d ", cmd->iec_func);
				return false;
			}
		}
		cmd->value->mem_ptr = (uint8_t*) malloc(size_in_bytes );
		if (cmd->value->mem_ptr == NULL)
		{
			slog_error("Unable to allocate %d bytes ",size_in_bytes);
			return false;
		}
		memset(cmd->value->mem_ptr, 0, size_in_bytes );

		cmd->value->mem_size = size_in_bytes;
		cmd->value->mem_state = mem_init;

	 return true;
 }

bool allocate_cmd_memory(Transl_Config_TypeDef *config, iec104_server *iec104_server)
{
	uint8_t iec104_slave_cnt = 0;
	for (int i = 0; i < config->num_ports; i++)	// Serial ports num
	{
		for (int j = 0; j < config->serialport[i].num_slaves; j++)// Modbus slaves num
		{
			for (int x = 0; x < config->serialport[i].mb_slave[j].mb_read_cmd_num; x++) // Modbus slave read commands num
			{
				data_mem *ptr = malloc(sizeof(data_mem) );
				config->serialport[i].mb_slave[j].mb_read_cmds[x].value = ptr;
				//config->serialport[i].mb_slave[j].iec104_read_cmds[x].value = ptr;
				iec104_server->iec104_slave[iec104_slave_cnt].iec104_read_cmds[x].value =ptr;
				allocate_read_cmd_memory(&config->serialport[i].mb_slave[j].mb_read_cmds[x] );
			}

			for (int x = 0; x < config->serialport[i].mb_slave[j].mb_write_cmd_num; x++) // Modbus slave write commands num
			{
				data_mem *ptr = malloc(sizeof(data_mem) );
				config->serialport[i].mb_slave[j].mb_write_cmds[x].value = ptr;
				//config->serialport[i].mb_slave[j].iec104_write_cmds[x].value = ptr;
				iec104_server->iec104_slave[iec104_slave_cnt].iec104_write_cmds[x].value =ptr;
				allocate_write_cmd_memory(&iec104_server->iec104_slave[iec104_slave_cnt].iec104_write_cmds[x] );
			}

			iec104_slave_cnt++;
		}
	}

	return true;
}

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

	size_t iec_fn_str_len = sizeof(iec_read_fn_str) / sizeof(iec_read_fn_str[0]);
	read_cmd->iec_func = 0;
	for (int fn=0; fn< iec_fn_str_len; fn++)
	{
		if ( !strcmp(str, iec_read_fn_str[fn].func_str) ) read_cmd->iec_func = iec_read_fn_str[fn].func_n;
	}
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

	size_t iec_fn_str_len = sizeof(iec_write_fn_str) / sizeof(iec_write_fn_str[0]);
	write_cmd->iec_func = 0;
	for (int fn=0; fn< iec_fn_str_len; fn++)
	{
		if ( !strcmp(str, iec_write_fn_str[fn].func_str) ) write_cmd->iec_func = iec_write_fn_str[fn].func_n;
	}
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

	json_object_object_get_ex(parsed_json, "read_commands", &read_cmd_json );
	int read_cmd_num = json_object_array_length(read_cmd_json );
	iec_slave->iec104_read_cmd_num =  read_cmd_num;

	iec_slave->iec104_read_cmds = (iec104_command*) malloc(iec_slave->iec104_read_cmd_num * sizeof(iec104_command) );

	for (uint8_t i = 0; i < iec_slave->iec104_read_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(read_cmd_json, i );
		if ( ! parse_iec104_read_cmd( cur_cmd, &iec_slave->iec104_read_cmds[i]) )
		{
			slog_error( "Parsing read commands failed.");
			return false;
		}
	}

	json_object_object_get_ex(parsed_json, "write_commands", &write_cmd_json );
	int write_cmd_num = json_object_array_length(write_cmd_json );
	iec_slave->iec104_write_cmd_num = write_cmd_num;
	iec_slave->iec104_write_cmds = (iec104_command*) malloc(iec_slave->iec104_write_cmd_num * sizeof(iec104_command) );
	for (int i = 0; i < iec_slave->iec104_write_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(write_cmd_json, i );
		if ( ! parse_iec104_write_cmd( cur_cmd, &iec_slave->iec104_write_cmds[i]) )
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

	json_object_object_get_ex(parsed_json, "read_commands", &read_cmd_json );
	int read_cmd_num = json_object_array_length(read_cmd_json );
	mb_slave->mb_read_cmd_num =  read_cmd_num;

	mb_slave->mb_read_cmds = (modbus_command*) malloc(mb_slave->mb_read_cmd_num * sizeof(modbus_command) );

	for (uint8_t i = 0; i < mb_slave->mb_read_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(read_cmd_json, i );
		if ( ! parse_modbus_read_cmd( cur_cmd, &mb_slave->mb_read_cmds[i]) )
		{
			slog_error( "Parsing read commands failed.");
			return false;
		}
	}

	json_object_object_get_ex(parsed_json, "write_commands", &write_cmd_json );
	int write_cmd_num = json_object_array_length(write_cmd_json );

	mb_slave->mb_write_cmd_num = write_cmd_num;
	mb_slave->mb_write_cmds = (modbus_command*) malloc(mb_slave->mb_write_cmd_num * sizeof(modbus_command) );
	for (int i = 0; i < mb_slave->mb_write_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(write_cmd_json, i );
		if ( ! parse_modbus_write_cmd( cur_cmd, &mb_slave->mb_write_cmds[i]) )
		{
			slog_error( "Parsing write commands failed.");
			return false;
		}
	}

	return true;
}
/*
bool parse_slave_config(struct json_object *parsed_json,Modbus_Slave_TypeDef *mb_slave, iec104_slave *iec_slave )
{
	struct json_object *read_cmd_json=NULL, *write_cmd_json=NULL, *cur_cmd=NULL;

	json_object_object_get_ex(parsed_json, "read_commands", &read_cmd_json );

	int read_cmd_num = json_object_array_length(read_cmd_json );
	mb_slave->mb_read_cmd_num =  read_cmd_num;
	iec_slave->iec104_read_cmd_num =  read_cmd_num;

	mb_slave->mb_read_cmds = (modbus_command*) malloc(mb_slave->mb_read_cmd_num * sizeof(modbus_command) );
	iec_slave->iec104_read_cmds = (iec104_command*) malloc(iec_slave->iec104_read_cmd_num * sizeof(iec104_command) );

	for (uint8_t i = 0; i < mb_slave->mb_read_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(read_cmd_json, i );
		if ( ! parse_modbus_read_cmd( cur_cmd, &mb_slave->mb_read_cmds[i]) )
		{
			slog_error( "Parsing read commands failed.");
			return false;
		}
	}
	for (uint8_t i = 0; i < iec_slave->iec104_read_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(read_cmd_json, i );
		if ( ! parse_iec104_read_cmd( cur_cmd, &iec_slave->iec104_read_cmds[i]) )
		{
			slog_error( "Parsing read commands failed.");
			return false;
		}
	}

	json_object_object_get_ex(parsed_json, "write_commands", &write_cmd_json );
	int write_cmd_num = json_object_array_length(write_cmd_json );

	mb_slave->mb_write_cmd_num = write_cmd_num;
	iec_slave->iec104_write_cmd_num = write_cmd_num;
	mb_slave->mb_write_cmds = (modbus_command*) malloc(mb_slave->mb_write_cmd_num * sizeof(modbus_command) );
	iec_slave->iec104_write_cmds = (iec104_command*) malloc(iec_slave->iec104_write_cmd_num * sizeof(iec104_command) );
	for (int i = 0; i < mb_slave->mb_write_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(write_cmd_json, i );
		if ( ! parse_modbus_write_cmd( cur_cmd, &mb_slave->mb_write_cmds[i]) )
		{
			slog_error( "Parsing write commands failed.");
			return false;
		}
	}
	for (int i = 0; i < iec_slave->iec104_write_cmd_num; i++)
	{
		cur_cmd = json_object_array_get_idx(write_cmd_json, i );
		if ( ! parse_iec104_write_cmd( cur_cmd, &iec_slave->iec104_write_cmds[i]) )
		{
			slog_error( "Parsing write commands failed.");
			return false;
		}
	}

	// free memory
//	json_object_put(parsed_json);
	return true;
}
*/

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

	// parse iec104_send_rate
	json_object_object_get_ex(parsed_json, "iec104_send_rate_s", &tmp_json );
	iec104_server->iec104_send_rate = json_object_get_int(tmp_json);

	json_object_object_get_ex(parsed_json, "serial_ports", &ports );
	config->num_ports = json_object_array_length(ports );
	config->serialport = (Serial_Port_TypeDef*) malloc(config->num_ports * sizeof(Serial_Port_TypeDef) );

	iec104_server->iec104_slave_num = 0;
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
		str = json_object_get_string(tmp_json );
		if (strlen(str ) == 0)
		{
			slog_warn("Serial port %d device not specified. for example '/dev/ttyS0' ", i );
//			return NULL;
		}
		config->serialport[i].device = (char*) malloc((strlen(str ) + 1) * sizeof(char) );
		strcpy(config->serialport[i].device, str );
		// port baudrate
		tmp_json = json_object_array_get_idx(port_device, cfg_port_baudrate);
		config->serialport[i].baud = json_object_get_int(tmp_json);
		// port data bit
		tmp_json = json_object_array_get_idx(port_device, cfg_port_data_bit);
		config->serialport[i].data_bit = json_object_get_int(tmp_json);
		// port parity
		tmp_json = json_object_array_get_idx(port_device, cfg_port_parity);
		str = json_object_get_string(tmp_json);
		config->serialport[i].parity = (char) *str;
		// port stop bit
		tmp_json = json_object_array_get_idx(port_device, cfg_port_stop_bit);
		config->serialport[i].stop_bit = json_object_get_int(tmp_json);

		// parse protocol
		json_object_object_get_ex(cur_port, "Protocol", &tmp_json );
		str = json_object_get_string(tmp_json);
		if ( !strcmp(str, "modbus_rtu_master") )  config->serialport[i].protocol = cfg_modbus_rtu_m;
		else if ( !strcmp(str, "modbus_rtu_slave") ) config->serialport[i].protocol = cfg_modbus_rtu_s;
		else if ( !strcmp(str, "iec_60870-5-101") ) config->serialport[i].protocol = cfg_iec_101;
		else if ( !strcmp(str, "iec_60870-5-103") ) config->serialport[i].protocol = cfg_iec_103;
		else
		{
			slog_error( "Wrong serial protocol: '%s' .", str);
			return false;
		}

		config->serialport[i].recv_timeout = json_object_get_int(tmp_json);


		// parse modbus answer timeout
		json_object_object_get_ex(cur_port, "answer_timeout_ms", &tmp_json );
		config->serialport[i].recv_timeout = json_object_get_int(tmp_json);


		// parse modbus slaves param
		json_object_object_get_ex(cur_port, "slave", &mb_slaves );
		config->serialport[i].num_slaves= json_object_array_length(mb_slaves );
		config->serialport[i].mb_slave = (Modbus_Slave_TypeDef*) malloc(config->serialport[i].num_slaves * sizeof(Modbus_Slave_TypeDef) );

		iec104_server->iec104_slave_num += config->serialport[i].num_slaves;
	}

	iec104_server->iec104_slave = (iec104_slave*) malloc(iec104_server->iec104_slave_num * sizeof(iec104_slave) );

	uint8_t iec104_slave_cnt = 0;
	for (int i = 0; i < config->num_ports; i++)
	{
		cur_port = json_object_array_get_idx(ports, i );

		// parse modbus slaves param
		json_object_object_get_ex(cur_port, "slave", &mb_slaves );

		for (int j = 0; j < config->serialport[i].num_slaves; j++)
		{
			cur_slave = json_object_array_get_idx(mb_slaves, j );
			// slave addr
			json_object_object_get_ex(cur_slave, "slave_address", &tmp_json );
			int slave_addr = json_object_get_int(tmp_json );
			config->serialport[i].mb_slave[j].mb_slave_addr = slave_addr;
			iec104_server->iec104_slave[iec104_slave_cnt].iec_asdu_addr = slave_addr;
			// slave config file
			json_object_object_get_ex(cur_slave, "slave_config_file", &tmp_json );
			str = json_object_get_string(tmp_json );


			// read and parse slave config file
			struct json_object *parsed_slave_json=NULL;
			if (!read_json_file(str,&parsed_slave_json) )	return false;

/*
			if (!parse_slave_config(parsed_slave_json, &config->serialport[i].mb_slave[j], &iec104_server->iec104_slave[iec104_slave_cnt++] ))
			{
				slog_error(" Loading : 'serial_ports:%d\\mb_slave:%d\\mb_slave_config_file: %s'  failed. ", i, j, str );
				return false;
			}
*/
			if (!parse_slave_iec104_config(parsed_slave_json, &iec104_server->iec104_slave[iec104_slave_cnt++] ))
			{
				slog_error(" Loading : 'serial_ports:%d\\slave:%d\\slave_config_file: %s'  failed. ", i, j, str );
				return false;
			}
			if (!parse_slave_modbus_config(parsed_slave_json, &config->serialport[i].mb_slave[j] ))
			{
				slog_error(" Loading : 'serial_ports:%d\\slave:%d\\slave_config_file: %s'  failed. ", i, j, str );
				return false;
			}
			json_object_put(parsed_slave_json);	// free
		}

	}



	allocate_cmd_memory(config, iec104_server);

	json_object_put(parsed_json);	// free
	slog_info( "Config file: '%s' successful read", filename);
	return true;
}



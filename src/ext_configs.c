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


 bool allocate_read_cmd_memory(Command_TypeDef *cmd)
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
		slog_error("Data size for read_command = 0, mb_fn=%d, ioa=%d ",  cmd->mb_func, cmd->iec_ioa_addr );
		return false;
	}
	cmd->mem_ptr = (uint8_t*) malloc(size_in_bytes );
	if (cmd->mem_ptr == NULL)
	{
		slog_error("Unable to allocate %d bytes for read_command, mb_fn=%d, ioa=%d ", size_in_bytes, cmd->mb_func, cmd->iec_ioa_addr );
		return false;
	}
	memset(cmd->mem_ptr, 0, size_in_bytes );

	cmd->mem_size = size_in_bytes;
	cmd->mem_state = mem_init;
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


/*
	json_object *jobj = NULL;
	const char *mystring = NULL;
	int stringlen = 0;
	enum json_tokener_error jerr;
*/
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


int parse_iec_add_params(struct json_object *add_parm_json, Command_TypeDef *cmd )
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
				else cmd->add_params.priority = cfg_prior_low;
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

bool parse_read_command(struct json_object *cur_cmd, Command_TypeDef *read_cmd )
{
	const char *str;
	struct json_object *tmp_json=NULL, *mb_data_json=NULL, *iec_data_json=NULL;

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
	if (json_object_object_get_ex(cur_cmd, "add_param", &add_parm_json ))
	{
		parse_iec_add_params( add_parm_json , read_cmd );
	}

	// Allocate and initialize the memory to store the registers
	if (! allocate_read_cmd_memory(read_cmd)) return false;

	return true;
}

int parse_write_command(struct json_object *cur_cmd, Command_TypeDef *write_cmd )
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


bool parse_slave_config_file(const char *filename,Modbus_Slave_TypeDef *slave )
{
	struct json_object *parsed_json=NULL;
	struct json_object *read_cmd_json=NULL, *write_cmd_json=NULL, *cur_cmd=NULL;


	if (!read_json_file(filename,&parsed_json) )
		return false;

	json_object_object_get_ex(parsed_json, "read_commands", &read_cmd_json );
	slave->read_cmnds_num = json_object_array_length(read_cmd_json );
	slave->read_cmnds = (Command_TypeDef*) malloc(slave->read_cmnds_num * sizeof(Command_TypeDef) );
	for (uint8_t i = 0; i < slave->read_cmnds_num; i++)
	{
		cur_cmd = json_object_array_get_idx(read_cmd_json, i );
		if ( ! parse_read_command( cur_cmd, &slave->read_cmnds[i]) )
		{
			slog_error( "Parsing read commands in file: '%s' failed.", filename);
			return false;
		}
	}

	json_object_object_get_ex(parsed_json, "write_commands", &write_cmd_json );
	slave->write_cmnds_num = json_object_array_length(write_cmd_json );
	slave->write_cmnds = (Command_TypeDef*) malloc(slave->write_cmnds_num * sizeof(Command_TypeDef) );
	for (int i = 0; i < slave->write_cmnds_num; i++)
	{
		cur_cmd = json_object_array_get_idx(write_cmd_json, i );
		if ( ! parse_write_command( cur_cmd, &slave->write_cmnds[i]) )
		{
			slog_error( "Parsing write commands in file: '%s' failed.", filename);
			return false;
		}
	}

	// free memory
	json_object_put(parsed_json);
	return true;
}


Transl_Config_TypeDef* read_config_file(const char *filename)
{

	Transl_Config_TypeDef *config = NULL;
	struct json_object *parsed_json=NULL;
	struct json_object *ports=NULL, *cur_port=NULL, *port_device=NULL , *mb_slaves=NULL, *cur_slave=NULL;

	struct json_object *tmp_json=NULL;
	const char *str;
	size_t tmp;

	slog_info( "Read main config file: '%s' ", filename);
	if (!read_json_file(filename,&parsed_json))
	{
		slog_error( "Read config file: '%s' failed.", filename);
		return NULL;
	}
	config = (Transl_Config_TypeDef*) malloc(sizeof(Transl_Config_TypeDef) );
	if (config == NULL)
	{
		slog_error( "Allocating memory for config file: '%s' failed.", filename);
		return NULL;
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
	config->iec104_send_rate = json_object_get_int(tmp_json);

	json_object_object_get_ex(parsed_json, "serial_ports", &ports );
	config->num_ports = json_object_array_length(ports );
	config->serialport = (Serial_Port_TypeDef*) malloc(config->num_ports * sizeof(Serial_Port_TypeDef) );

	for (int i = 0; i < config->num_ports; i++)
	{
		cur_port = json_object_array_get_idx(ports, i );

		// parse serial port param
		json_object_object_get_ex(cur_port, "device", &port_device );
		tmp = json_object_array_length(port_device );
		if (tmp != cfg_port_el_num)
		{
			slog_error( "Wrong number of elements in: 'serial_ports:%d\\device' section", i );
			return NULL;
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

		// parse modbus answer timeout
		json_object_object_get_ex(cur_port, "mb_answer_timeout_ms", &tmp_json );
		config->serialport[i].recv_timeout = json_object_get_int(tmp_json);


		// parse modbus slaves param
		json_object_object_get_ex(cur_port, "mb_slave", &mb_slaves );
		config->serialport[i].num_slaves= json_object_array_length(mb_slaves );
		config->serialport[i].mb_slave = (Modbus_Slave_TypeDef*) malloc(config->serialport[i].num_slaves * sizeof(Modbus_Slave_TypeDef) );

		for (int j = 0; j < config->serialport[i].num_slaves; j++)
		{
			cur_slave = json_object_array_get_idx(mb_slaves, j);
			// slave addr
			json_object_object_get_ex(cur_slave, "mb_slave_address", &tmp_json );
			config->serialport[i].mb_slave[j].mb_slave_addr = json_object_get_int(tmp_json);
			// slave config file
			json_object_object_get_ex(cur_slave, "mb_slave_config_file", &tmp_json );
			str = json_object_get_string(tmp_json);
			if ( !parse_slave_config_file(str, &config->serialport[i].mb_slave[j]))
			{
				slog_error( " Loading : 'serial_ports:%d\\mb_slave:%d\\mb_slave_config_file: %s'  failed. ", i, j,str );
				return NULL;
			}
		}
	}

	json_object_put(parsed_json);	// free
	slog_info( "Config file: '%s' successful read", filename);
	return config;
}



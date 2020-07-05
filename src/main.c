/*
 * main.c
 *
 *  Created on: 22 мая 2020 г.
 *      Author: kulish_y
 */
#include "main.h"

static bool running = true;




void sigint_handler(int signalId)
{
    running = false;
}

int main(int argc, char *argv[])
{
	Transl_Config_TypeDef	*config;
	CS104_Slave slave = NULL;

	moxa_buzzer(100);
	bool mb_debug = false, iec_debug = false;
	if (argc>1)
	{
		if ( !strcmp(argv[1], "mb_debug") ) mb_debug= true;
		else if ( !strcmp(argv[1], "iec_debug") ) iec_debug= true;
	}

	// Add Ctrl-C handler
	signal(SIGINT, sigint_handler );

	// Initialize slog
	slog_init("logs/iec2modbus", NULL, SL_LIVE, SL_LIVE);
	slog_info( "Start  Version %d.%d build %s, %s.",VERSION_MAJOR, VERSION_MINOR,  __DATE__, __TIME__);


	// read main config
	config = read_config_file("config.json" );
	if (config == NULL)
	{
		slog_error( "Config file load failed.");
		exit(EXIT_FAILURE );
	}

	SlogConfig pCfg;
	slog_config_get(&pCfg);
	pCfg.nLogLevel = config->log_level;
	pCfg.nFileLevel = config->log_level;
	slog_config_set(&pCfg);

/*
	if (allocate_read_cmd_memory(config) !=0 )
	{
		slog_error( "Failed allocate memory for read commands.");
		exit(EXIT_FAILURE );
	}

	if (allocate_write_cmd_memory(config) !=0 )
	{
		slog_error( "Failed allocate memory for write commands.");
		exit(EXIT_FAILURE );
	}
*/


	// start modbus master
	for (uint8_t i = 0; i < config->num_ports; i++)
	{
		if (Modbus_Init(&config->serialport[i], mb_debug) == 0)
		{
			Modbus_Thread_Start(&config->serialport[i] );
			slog_info( "Start modbus on serial port %s", config->serialport[i].device);
		}
		 else
			 slog_warn("Modbus master on serial port %s not started ",config->serialport[i].device );
	}


	slave = iec104_server_init(config, iec_debug);
	CS104_Slave_start(slave );
	if (CS104_Slave_isRunning(slave ) == false)
	{
		slog_error( "Starting iec104 server failed!");
		exit(EXIT_FAILURE );
	}
	slog_info( "Start iec104 server.");


	moxa_buzzer(500);

	int iec_send_timer = 0;
	while (running)
	{
		iec104_send_changed_data(slave, config, cfg_prior_hight);
		if (iec_send_timer == config->iec104_send_rate)
		{
			iec104_send_changed_data(slave, config, cfg_prior_low);
			iec104_send_moxa_dio( slave);
			iec_send_timer = 0;
		}
		iec_send_timer++;
		Thread_sleep(1000 );
	}

	slog_info( "Stop iec104 server");
	CS104_Slave_stop(slave );
	CS104_Slave_destroy(slave );

	for (uint8_t i = 0; i < config->num_ports; i++)
	{
		slog_info( "Stop modbus on serial port %s", config->serialport[i].device);
		Modbus_Thread_Stop(&config->serialport[i]);
	}


	for (int i = 0; i < config->num_ports; i++)	// Serial ports num
	{
		for (int j = 0; j < config->serialport[i].num_slaves; j++)// Modbus slaves num
		{
			for (int x = 0; x < config->serialport[i].mb_slave[j].mb_write_cmd_num; x++) // Modbus slave write commands num
			{
				free(config->serialport[i].mb_slave[j].mb_write_cmds[x].value->mem_ptr);
				free(config->serialport[i].mb_slave[j].mb_write_cmds[x].value);
			}
		}
	}

	for (int i = 0; i < config->num_ports; i++)	// Serial ports num
	{
		for (int j = 0; j < config->serialport[i].num_slaves; j++)// Modbus slaves num
		{
			for (int x = 0; x < config->serialport[i].mb_slave[j].mb_read_cmd_num; x++) // Modbus slave write commands num
			{
				free(config->serialport[i].mb_slave[j].mb_read_cmds[x].value->mem_ptr);
				free(config->serialport[i].mb_slave[j].mb_read_cmds[x].value);
			}
		}
	}


	for (int i = 0; i < config->num_ports; i++)	// Serial ports num
	{
		for (int j = 0; j < config->serialport[i].num_slaves; j++)// Modbus slaves num
		{
				free(config->serialport[i].mb_slave[j].mb_read_cmds);
				free(config->serialport[i].mb_slave[j].mb_write_cmds);
				free(config->serialport[i].mb_slave[j].iec104_read_cmds);
				free(config->serialport[i].mb_slave[j].iec104_write_cmds);
		}
	}

	for (int i = 0; i < config->num_ports; i++)	// Serial ports num
	{
		free(config->serialport[i].mb_slave);
		free(config->serialport[i].device);
	}

	free(config->serialport);
	free(config);



	slog_warn( "Stop programm. \n");

	Thread_sleep(500 );
	return 0;
}


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
	Transl_Config_TypeDef	config;
	iec104_server*			iec104_server;


	buzzer_on(100);
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


	// iec104 server create and init
	iec104_server = iec104_server_init( iec_debug);

	// read main config
	if (! read_config_file("config.json",&config ,iec104_server ))
	{
		slog_error( "Config file load failed.");
		exit(EXIT_FAILURE );
	}

	SlogConfig pCfg;
	slog_config_get(&pCfg);
	pCfg.nLogLevel = config.log_level;
	pCfg.nFileLevel = config.log_level;
	slog_config_set(&pCfg);



	for (uint8_t i = 0; i < config.num_ports; i++)
	{
		switch (config.virt_port[i].protocol)
		{
			case cfg_modbus_rtu_m:		// start modbus master
			{
				if (Modbus_Init(config.virt_port[i].serial_port, (Modbus_Master*) config.virt_port[i].protocol_ptr, mb_debug ) == 0)
				{
					Modbus_Thread_Start( (Modbus_Master*) config.virt_port[i].protocol_ptr );
					slog_info("Start modbus on serial port %s", config.virt_port[i].serial_port->interfaceName);
				} else
					slog_warn("Modbus master on serial port %s not started ", config.virt_port[i].serial_port->interfaceName );
			}break;
			case cfg_modbus_rtu_s:
			{

			}break;
			case cfg_iec_101:
			{

			}break;
			case cfg_iec_103:
			{

			}break;
		}
	}


	//slave = iec104_server_init(&config->iec104_server, iec_debug);

	CS104_Slave_start(iec104_server->server );
	if (CS104_Slave_isRunning(iec104_server->server ) == false)
	{
		slog_error( "Starting iec104 server failed!");
		exit(EXIT_FAILURE );
	}
	slog_info( "Start iec104 server.");


	buzzer_on(500);

	int iec_send_timer = 0;
	while (running)
	{
		iec104_send_changed_data(iec104_server, cfg_prior_hight);
		if (iec_send_timer == iec104_server->iec104_send_rate)
		{
			iec104_send_changed_data(iec104_server, cfg_prior_low);
			iec104_send_dio( iec104_server->server);
			iec_send_timer = 0;
		}
		iec_send_timer++;
		Thread_sleep(1000 );
	}


	// stop serials protocols
	for (uint8_t i = 0; i < config.num_ports; i++)
	{
		switch (config.virt_port[i].protocol)
		{
			case cfg_modbus_rtu_m:
			{
				slog_info( "Stop modbus on serial port %s", config.virt_port[i].serial_port->interfaceName);
				Modbus_Thread_Stop((Modbus_Master*) config.virt_port[i].protocol_ptr);
			}break;
			case cfg_modbus_rtu_s:
			{

			}break;
			case cfg_iec_101:
			{

			}break;
			case cfg_iec_103:
			{

			}break;
		}
	}


	iec104_server_stop( iec104_server);
	slog_info( "Stop iec104 server");


	for (int i = 0; i < config.num_ports; i++)	// Serial ports num
	{
		SerialPort_destroy(config.virt_port[i].serial_port);
	}
	free(config.virt_port);


	slog_warn( "Stop programm. \n");

	Thread_sleep(50 );
	return 0;
}


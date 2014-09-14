#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include "abb_modbus.h"
#include "common.h"

static uint8_t gotsigint = 0;
uint16_t *inputs_raw;

void siginthandler()
{
	gotsigint = 1;
}

int main (int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./%s <serial_device>\n", argv[0]);
		return -1;
	}

	modbus_t *modbusport;
  	int i;

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(REG_READ_COUNT * sizeof(uint16_t));
	memset(inputs_raw, 0, REG_READ_COUNT * sizeof(uint16_t));

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	modbusport = abb_modbus_init(argv[1]);	

	write(1, BANNER, strlen(BANNER));

	while(1)
	{
		if (abb_update_input_registers(inputs_raw, modbusport))
		{
		}	
		/* if we caught sigint, close modbus
		   connections & exit gracefully */
		if (gotsigint) fail("Closing modbus connections & exiting.", modbusport); 
	}
}

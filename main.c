#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include "abb_modbus.h"
#include "common.h"

static uint8_t gotsigint = 0;

void siginthandler()
{
	gotsigint = 1;
}

struct values
{
	uint16_t *inputs_raw;
	float *inputs_scaled;
};

int main ( int argc, char *argv[] )
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./%s <serial_device>\n", argv[0]);
		return -1;
	}

	modbus_t *modbusport;
  	int i, ret;
	struct values *registers, r;
	registers = &r;

	/* Allocate space to store register reads */
	registers->inputs_raw = (uint16_t *) malloc(INPUT_REG_READ_COUNT * sizeof(uint16_t));
	memset(registers->inputs_raw, 0, INPUT_REG_READ_COUNT * sizeof(uint16_t));

	registers->inputs_scaled = (float *) malloc(INPUT_REG_READ_COUNT * sizeof(float));
	memset(registers->inputs_scaled, 0, INPUT_REG_READ_COUNT * sizeof(float));

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	modbusport = abb_modbus_init(argv[0]);	

	write(1, BANNER, strlen(BANNER));

	while(1)
	{
		ret = abb_update_input_registers(registers->inputs_raw, registers->inputs_scaled, modbusport);	
		/* if we caught sigint, close modbus
		   connections & exit gracefully */
		if (gotsigint) fail("Closing modbus connections & exiting.", modbusport); 
	}
}

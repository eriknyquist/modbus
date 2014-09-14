#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include "abb_modbus.h"
#include "abb_time.h"
#include "common.h"

static uint8_t gottimersig = 0;
static uint8_t gotsigint = 0;
uint16_t *inputs_raw;

void siginthandler()
{
	gotsigint = 1;
}

void timersighandler()
{
	gottimersig = 1;
}

int main (int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <serial_device>\n", argv[0]);
		return -1;
	}

	modbus_t *modbusport;
  	int i;

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(REG_READ_COUNT * sizeof(uint16_t));
	memset(inputs_raw, 0, REG_READ_COUNT * sizeof(uint16_t));

	/* Catch timer signal */
	signal(SIG, timersighandler);

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	modbusport = abb_modbus_init(argv[1]);	

	write(1, BANNER, strlen(BANNER));

	long long delaytime_ns = 1000000000 / UPDATE_FREQUENCY_HZ;
	start_interval_timer(delaytime_ns, SIG);	

	while(1)
	{
		if (gottimersig)
		{
			abb_update_input_registers(inputs_raw, modbusport);
			gottimersig = 0;
		}

		/* if we caught sigint, close modbus
		   connections & exit gracefully */
		if (gotsigint) fail("Closing modbus connections & exiting.", modbusport); 
	}
}

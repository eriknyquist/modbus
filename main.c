#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include "abb_pch550_modbus.h"
#include "abb_pch550_time.h"
#include "common.h"

static uint8_t gotsigint = 0;
uint16_t *inputs_raw;

void siginthandler()
{
	gotsigint = 1;
}

int main (int argc, char *argv[])
{
	parse_conf();
	exit(1);
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <serial_device>\n", argv[0]);
		return -1;
	}

	modbus_t *modbusport;
  	int i;
	uint8_t skip = 0;
	long long start;
	long long finish;
	long long remaining_usecs;
	long delaytime_ms = 1000 / UPDATE_FREQUENCY_HZ;

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(REG_READ_COUNT * sizeof(uint16_t));
	memset(inputs_raw, 0, REG_READ_COUNT * sizeof(uint16_t));

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	ile_aip_init();
	modbusport = abb_pch550_modbus_init(argv[1]);	

	write(1, BANNER, strlen(BANNER));

	while(1)
	{
		start = getms();

		/* if we caught sigint, close modbus
		   connections & exit gracefully */
		if (gotsigint)
			fail("Closing modbus connections & exiting.", modbusport); 

		if (skip)
			skip = 0;
		else
		{
			abb_pch550_read(inputs_raw, modbusport);
			write_registers_tofile(modbusport);
		}

		finish = getms();
		remaining_usecs = (delaytime_ms - (finish - start)) * 1000;

		if (finish >= start)
			usleep(remaining_usecs);
		else
			skip = 1;
	}
}

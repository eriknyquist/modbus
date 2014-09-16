#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include "abb_pch550_modbus.h"
#include "abb_pch550_time.h"
#include "common.h"

#define SENSORFILE "log.out"

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
		fprintf(stderr, "Usage: %s <serial_device>\n", argv[0]);
		return -1;
	}

	modbus_t *modbusport;
  	int i;
	long long start;
	long long finish;
	long long remaining_usecs;
	long delaytime_ms = 1000 / UPDATE_FREQUENCY_HZ;

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(REG_READ_COUNT * sizeof(uint16_t));
	memset(inputs_raw, 0, REG_READ_COUNT * sizeof(uint16_t));

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	modbusport = abb_pch550_modbus_init(argv[1]);	

	write(1, BANNER, strlen(BANNER));

	while(1)
	{
		/* if we caught sigint, close modbus
		   connections & exit gracefully */
		if (gotsigint)
			fail("Closing modbus connections & exiting.", modbusport); 

		start = getms();
		abb_pch550_read(inputs_raw, modbusport);
		write_registers_tofile(SENSORFILE, modbusport);
		finish = getms();
		remaining_usecs = (delaytime_ms - (finish - start)) * 1000;

		/* ----debug---- */
		printf("  sleeping for %lld out of %ld ms",
			remaining_usecs / 1000,
			delaytime_ms);
		/* ------------- */

		usleep(remaining_usecs);
	}
}

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <modbus.h>
#include "abb_pch550_modbus.h"
#include "abb_pch550_time.h"
#include "common.h"

int gotsigint = 0;
extern int modbus_read_count;
extern float update_frequency_hz;

uint16_t *inputs_raw;

void siginthandler()
{
	gotsigint = 1;
}

int main (int argc, char *argv[])
{
	modbus_t *modbusport;
  	int i;
	uint8_t skip = 0;
	uint64_t start;
	uint64_t finish;
	uint64_t offset;
	uint64_t previous;
	uint64_t remaining_usecs;
	uint64_t delaytime_us;

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	ile_aip_init();
	modbusport = abb_pch550_modbus_init();

	delaytime_us = (uint64_t) llrintf(1000000.0 / update_frequency_hz);

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(modbus_read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, modbus_read_count * sizeof(uint16_t));

	previous = getms() - (delaytime_us);
	while(1)
	{
		start = getms();

		if (gotsigint)
			fail("Closing modbus connections & exiting.", modbusport); 

		abb_pch550_read(inputs_raw, modbusport);
		write_registers_tofile(modbusport);

		offset = ((start - previous) > delaytime_us)
		? (start - previous) - delaytime_us : 0;

		previous = start;
		
		finish = getms();

		remaining_usecs = ((start + delaytime_us) - finish) - offset;
		usleep(remaining_usecs);

	}
}

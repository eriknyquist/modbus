#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <modbus.h>
#include "abb_ach550_modbus.h"
#include "abb_ach550_parse.h"
#include "abb_ach550_time.h"

int gotsigint = 0;

uint16_t *inputs_raw;

modbusport mbport = { .rtu_baud=9600,
                           .station_id=0,
                           .read_base=0,
                           .read_count=1,
                           .update_freq_hz=2,
                           .port_name="/dev/null" };
modbusport *mbp = &mbport;

void siginthandler()
{
	gotsigint = 1;
}

int main (int argc, char *argv[])
{
  	int i;
	uint8_t skip = 0;
	uint64_t start, finish, offset, previous, remaining_usecs, delaytime_us;

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	ile_aip_init();
	abb_ach550_modbus_init(mbp);

	/* derive total cycle time in microsecs from modbus_frequency_hz  */
	delaytime_us = (uint64_t) llrintf(1000000.0 / mbp->update_freq_hz);

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(mbp->read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, mbp->read_count * sizeof(uint16_t));

	/* must initialise a 'fake' previous timestamp,
	 * so the loop works first time */
	previous = getms() - (delaytime_us);
	while(1)
	{
		start = getms();

		if (gotsigint)
			fail("Closing modbus connections & exiting.", mbp->port); 

		if (skip)
			skip = 0;
		else
		{
			/* This is where all the 'actual work' goes- everything else
		 	* is just timer maintenance. You can add as much stuff as you want
		 	* and the cycle time should remain fixed (provided the actual execution
		 	* time does not exceed the cycle time) */

			/* -----THE ACTUAL WORK----- */

			abb_ach550_read(inputs_raw, mbp);
			write_registers_tofile(mbp);

			/* ------------------------- */
		}

		offset = ((start - previous) > delaytime_us)
			? (start - previous) - delaytime_us : 0;

		previous = start;
		finish = getms();
		
		if (finish >= start)
		{
			remaining_usecs = ((start + delaytime_us) - finish) - offset;
			usleep(remaining_usecs);
		}
		else
			skip = 1;
	}
}

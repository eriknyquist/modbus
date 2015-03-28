#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <modbus.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/signal.h>
#include "modbus_init.h"
#include "parse.h"
#include "time.h"
#include "read.h"

timer_t timer_id;
int gotsigint = 0;
uint16_t *inputs_raw;
element *pv;

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
	uint64_t interval;

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	ile_aip_init(mbp);
	get_modbus_params(mbp);
	pv = malloc(sizeof(element) * mbp->read_count);
	modbus_init(mbp, pv);


	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(mbp->read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, mbp->read_count * sizeof(uint16_t));

	while(1)
	{
		if (gotsigint)
			fail("Closing modbus connections & exiting.", mbp->port); 
		else
		{
			abb_ach550_read(inputs_raw, mbp, pv);
			write_registers_tofile(mbp, pv);
		}
	}
}

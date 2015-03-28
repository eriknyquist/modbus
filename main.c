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

int gotsigint = 0;

uint16_t *inputs_raw;
element *pv;

modbusport mbport = { .rtu_baud=9600,
                           .station_id=0,
                           .read_base=0,
                           .read_count=1,
                           .secs=2,
                           .port_name="/dev/null" };
modbusport *mbp = &mbport;

void siginthandler()
{
	gotsigint = 1;
}

void read_thread(void)
{
	abb_ach550_read(inputs_raw, mbp, pv);
	write_registers_tofile(mbp, pv);
}

int main (int argc, char *argv[])
{
  	int i, tstatus;

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	ile_aip_init(mbp);
	get_modbus_params(mbp);
	pv = malloc(sizeof(element) * mbp->read_count);
	modbus_init(mbp, pv);
	
	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(mbp->read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, mbp->read_count * sizeof(uint16_t));

	tstatus = create_periodic(mbp->secs, read_thread);
	if (tstatus == -1)
		fail("Error creating timer", mbp->port);

	while(1)
	{
		if(gotsigint)
			fail("Closing modbus connection...", mbp->port);
		usleep(10000);
	}
}

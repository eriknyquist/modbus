#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include <libgen.h>
#include "modbus_init.h"
#include "parse.h"
#include "time.h"
#include "read.h"

int gotsigint = 0;
char *dname;

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
	dname = basename(argv[0]);
	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	strncpy(mbp->dname, dname, sizeof(mbp->dname));
	ile_aip_init(mbp);
	get_modbus_params(mbp);
	pv = malloc(sizeof(element) * mbp->read_count);
	modbus_init(mbp, pv);
	
	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(mbp->read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, mbp->read_count * sizeof(uint16_t));

	tstatus = create_periodic(mbp->secs, read_thread);
	if (tstatus == -1)
		fatal("can't create timer", mbp);

	while(1)
	{
		if(gotsigint)
		{
			logger("closing modbus connection...", mbp);
			modbus_close(mbp->port);
			modbus_free(mbp->port);
			exit(0);
		}
		usleep(10000);
	}
}

/* Modbus logging daemon
 * Copyright (C) 2015 Erik Nyquist <eknyquist@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include <sys/types.h>
#include <libgen.h>
#include <signal.h>
#include <errno.h>
#include "shared.h"
#include "init.h"
#include "confparse.h"
#include "argparse.h"
#include "time.h"
#include "read.h"
#include "log.h"

volatile int gotkillsig = 0;
char *dname;

/* this is where mbd_read will put scaled 
 * register reads. */
element *pv;

modbusport mbport = {
	.rtu_baud =    DEFAULT_BAUD,
	.station_id =  DEFAULT_STATION_ID,
	.read_base =   DEFAULT_READ_BASE,
	.read_count =  DEFAULT_READ_COUNT,
	.msecs =       DEFAULT_MSECS,
	.port_name =   {DEFAULT_PORT_NAME}
};

logging loginfo = {
	.verbosity =    DEFAULT_VERBOSITY,
	.logdir =       {DEFAULT_LOGDIR},
	.uuidfile =     {DEFAULT_UUID_FILE},
	.sens_logdir =  {DEFAULT_SENS_LOGDIR},
	.conffile =     {DEFAULT_CONF_FILE}
};

modbusport *mbp = &mbport;
logging *lgp = &loginfo;

void siginthandler()
{
	gotkillsig = 1;
}

void mbd_tick(void)
{
	mbd_read(mbp, pv, lgp);
	write_registers_tofile(mbp, pv, lgp);
}

int main (int argc, char *argv[])
{
	if (argc > 1)
		parse_args(argc, argv, lgp);

#ifndef NOFORK
	pid_t pid = 0;

	pid = fork();
	if (pid < 0) {
		int er = errno;
		fprintf(stderr, "fork failed.\n");
		exit(er);
	}

	if (pid > 0)
		exit(0);
#endif

  	int tstatus;

	/* Catch sigint (ctrl-c) */
	signal(SIGINT, siginthandler);

	/* Catch sigterm (kill) */
	signal(SIGTERM, siginthandler);

	/* get executable name, used for logging */
	dname = basename(argv[0]);
	strncpy(lgp->dname, dname, sizeof(lgp->dname));

	/* get PID, also used for logging */
	lgp->pid = (unsigned long) getpid();

	/* initialise modbus & application parameters, & set
	 * up modbus port */
	pv = mbd_init(mbp, lgp);

	/* kick things off with an initial reading (otherwise
	 * the first reading won't happen until the first timer
	 * expires) */
	mbd_tick();

	tstatus = create_periodic(mbp->msecs, mbd_tick);
	if (tstatus != 0)
		fatal("can't create timer", mbp, lgp, tstatus);

	while(1) {
		if(gotkillsig) {
			free(pv);
			mbd_exit(mbp, lgp);
			exit(0);
		}
		usleep(10000);
	}
}

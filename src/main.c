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
#include <fcntl.h>
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
volatile int gotsigusr1 = 0;
char *dname;

/* this is where mbd_read will put scaled
 * register reads. */
element *pv;

mbdport mbport = {
	.rtu_baud =        DEFAULT_BAUD,
	.station_id =      DEFAULT_STATION_ID,
	.read_base =       DEFAULT_READ_BASE,
	.read_count =      DEFAULT_READ_COUNT,
	.msecs =           DEFAULT_MSECS,
	.port_name =       {DEFAULT_PORT_NAME},
	.maxretries =      DEFAULT_MAX_RETRIES,
	.retries =         0
};

logging loginfo = {
	.verbosity =       DEFAULT_VERBOSITY,
	.logdir =          {DEFAULT_LOGDIR},
	.sens_logdir =     {DEFAULT_SENS_LOGDIR},
};

mbdinfo minfo = {
	.uuidfile =        {DEFAULT_UUID_FILE},
	.conffile =        {DEFAULT_CONF_FILE},
	.monitor =         DEFAULT_MONITOR,
	.shouldfork =      DEFAULT_SHOULDFORK
};

mbdport *mbp = &mbport;
logging *lgp = &loginfo;
mbdinfo *mip = &minfo;

void siginthandler()
{
	gotkillsig = 1;
}

void sigusr1handler()
{
	gotsigusr1 = 1;
}

void mbd_tick (void)
{
	int ret;

	/* read modbus registers */
	ret = mbd_read(mbp, pv, lgp, mip);

	/* if read was successful, and if not in monitor
	 * mode, write readings to sensor log file. */
	if (!mip->monitor && ret == 0)
		write_registers_tofile(mbp, pv, lgp, mip);
}

void read_stdin (void)
{
	size_t n;
	char buf[80];
	int fd;

	fd = open(CONTROL_FIFO_PATH, O_RDONLY);
	n = read(fd, &buf, sizeof(buf) - 1);
	close(fd);

	buf[n] = '\0';

	printf("got \"%s\" from FIFO!\n", buf);
}

int main(int argc, char *argv[])
{
	if (argc > 1)
		parse_args(argc, argv, lgp, mip);

	if (mip->shouldfork) {
		pid_t pid = 0;

		pid = fork();
		if (pid < 0) {
			int er = errno;

			fprintf(stderr, "fork failed.\n");
			exit(er);
		}

		if (pid > 0)
			exit(0);
	}

	int tstatus;

	/* get executable name, used for logging */
	dname = basename(argv[0]);
	strncpy(mip->dname, dname, sizeof(mip->dname));

	/* get PID, also used for logging */
	mip->pid = (unsigned long) getpid();

	/* initialise modbus & application parameters, & set
	 * up modbus port */
	pv = mbd_init(mbp, lgp, mip);

	/* kick things off with an initial reading (otherwise
	 * the first reading won't happen until the first timer
	 * expires) */
	mbd_tick();

	tstatus = start_periodic_task(mbp->msecs, mbd_tick);
	if (tstatus != 0)
		fatal("can't create timer", mbp, lgp, mip, tstatus);

	/* Catch sigint (ctrl-c) */
	signal(SIGINT, siginthandler);

	/* Catch sigterm (kill) */
	signal(SIGTERM, siginthandler);

	/* Catch SIGUSR1, for writing control data */
	signal(SIGUSR1, sigusr1handler);

	while (1) {
		/* All the work is done in the mbd_tick routine, in a new
		 * thread spawned by timer_create (called by start_periodic_task).
		 * The only thing the main thread needs to do is check
		 * for a received kill signal every 100ms, and sleep
		 * the rest of the time. */
		if (gotsigusr1 == 1) {
			read_stdin();
			gotsigusr1 = 0;
		}

		if (gotkillsig == 1) {
			free(pv);
			mbd_exit(mbp, lgp, mip);
			exit(0);
		}

		usleep(50000);
	}
}

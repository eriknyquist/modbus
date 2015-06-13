#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include <sys/types.h>
#include <libgen.h>
#include <signal.h>
#include "init.h"
#include "parse.h"
#include "time.h"
#include "read.h"
#include "log.h"

#define DEFAULT_BAUD 9600
#define DEFAULT_STATION_ID 0
#define DEFAULT_READ_BASE 0
#define DEFAULT_READ_COUNT 1
#define DEFAULT_SECS 2
#define DEFAULT_PORT_NAME "/dev/null"
#define DEFAULT_LOGDIR 0

volatile int gotkillsig = 0;
char *dname;

/* this is where mbd_read will put scaled 
 * register reads. */
element *pv;

modbusport mbport = {
	.rtu_baud=   DEFAULT_BAUD,
	.station_id= DEFAULT_STATION_ID,
	.read_base=  DEFAULT_READ_BASE,
	.read_count= DEFAULT_READ_COUNT,
	.secs=       DEFAULT_SECS,
	.port_name=  {DEFAULT_PORT_NAME},
	.logdir[0]=     '\0'};

modbusport *mbp = &mbport;

void siginthandler()
{
	gotkillsig = 1;
}

void read_thread(void)
{
	mbd_read(mbp, pv);
	write_registers_tofile(mbp, pv);
}

int main (int argc, char *argv[])
{
	pid_t pid = 0;

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork failed.\n");
		exit(-1);
	}

	if (pid > 0)
		exit(0);

  	int tstatus;

	/* Catch sigint (ctrl-c) */
	signal(SIGINT, siginthandler);

	/* Catch sigterm (kill) */
	signal(SIGTERM, siginthandler);

	/* get executable name, used for logging */
	dname = basename(argv[0]);
	strncpy(mbp->dname, dname, sizeof(mbp->dname));

	/* get PID, also used for logging */
	mbp->pid = (unsigned long) getpid();

	pv = mbd_init(mbp);

	tstatus = create_periodic(mbp->secs, read_thread);
	if (tstatus == -1)
		fatal("can't create timer", mbp);

	while(1) {
		if(gotkillsig) {
			free(pv);
			mbd_exit(mbp);
			exit(0);
		}
		usleep(10000);
	}
}

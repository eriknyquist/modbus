#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>
#include <sys/types.h>
#include <libgen.h>
#include <signal.h>
#include "shared.h"
#include "init.h"
#include "parse.h"
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
	.secs =        DEFAULT_SECS,
	.port_name =   {DEFAULT_PORT_NAME}
};

logging loginfo = {
	.logdir =      {DEFAULT_LOGDIR},
	.uuidfile =    {DEFAULT_UUID_FILE},
	.sens_logdir = {DEFAULT_SENS_LOGDIR}
};

modbusport *mbp = &mbport;
logging *lgp = &loginfo;

void siginthandler()
{
	gotkillsig = 1;
}

void read_thread(void)
{
	mbd_read(mbp, pv, lgp);
	write_registers_tofile(mbp, pv, lgp);
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
	strncpy(lgp->dname, dname, sizeof(lgp->dname));

	/* get PID, also used for logging */
	lgp->pid = (unsigned long) getpid();

	pv = mbd_init(mbp, lgp);

	tstatus = create_periodic(mbp->secs, read_thread);
	if (tstatus == -1)
		fatal("can't create timer", mbp, lgp);

	while(1) {
		if(gotkillsig) {
			free(pv);
			mbd_exit(mbp, lgp);
			exit(0);
		}
		usleep(10000);
	}
}

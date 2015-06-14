#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "init.h"
#include "time.h"

void err (char *errstr, logging *lp)
{
	char msg[MAX_LOG_LEN];
	FILE *fp;

	snprintf(msg, sizeof(msg), "[%s][%s.%ld] error: %s %s",
			timestamp(), lp->dname, lp->pid, errstr, strerror(errno));

	/* if log location not defined or inaccessible, print to stderr */
	if (strlen(lp->logdir) == 0 || (fp = fopen(lp->errfile, "a")) == NULL) {
		fprintf(stderr, "%s\n", msg);
	} else {
		fprintf(fp, "%s\n", msg);	
		fclose(fp);
	}
}

void logger (char *str, logging *lp)
{
	char msg[MAX_LOG_LEN];
	FILE *fp;

	/* if log location not defined or inaccessible, print to stdout */
	snprintf(msg, sizeof(msg), "[%s][%s.%ld] log: %s",
			timestamp(), lp->dname, lp->pid, str);

	if (strlen(lp->logdir) == 0 || (fp = fopen(lp->logfile, "a")) == NULL) {
		fprintf(stdout, "%s\n", msg);
	} else {
		fprintf(fp, "%s\n", msg);	
		fclose(fp);
	}
}

void fatal (char * errstr, modbusport *mp, logging *lp)
{
	err(errstr, lp);

	if (mp != NULL) {
		modbus_close(mp->port);
		modbus_free(mp->port);
	}

	err("exiting.", lp);
	exit(-1);
}

void log_init(logging *lp)
{
	/* if log location has been defined in conf file, create logfile names
	 * based on executable name & PID */
	if (strlen(lp->logdir) > 0) {
		snprintf(lp->logfile, sizeof(lp->logfile),
			"%s/%s-log.%ld", lp->logdir, lp->dname, lp->pid);

		snprintf(lp->errfile, sizeof(lp->errfile),
			"%s/%s-err.%ld", lp->logdir, lp->dname, lp->pid);
	}
}

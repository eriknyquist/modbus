#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "init.h"
#include "time.h"

void err (char *errstr, modbusport *mp)
{
	char msg[MAX_LOG_LEN];
	FILE *fp;

	snprintf(msg, sizeof(msg), "[%s][%s.%ld]:error: %s %s",
			timestamp(), mp->dname, mp->pid, errstr, strerror(errno));

	if (mp->logdir == 0 || (fp = fopen(mp->errfile, "a")) == NULL)
		fprintf(stderr, "%s\n", msg);
	else
	{
		fprintf(fp, "%s\n", msg);	
		fclose(fp);
	}
}

void logger (char *str, modbusport *mp)
{
	char msg[MAX_LOG_LEN];
	FILE *fp;

	snprintf(msg, sizeof(msg), "[%s][%s.%ld]:log: %s",
			timestamp(), mp->dname, mp->pid, str);

	if (mp->logdir == 0 || (fp = fopen(mp->logfile, "a")) == NULL)
		fprintf(stdout, "%s\n", msg);
	else
	{
		fprintf(fp, "%s\n", msg);	
		fclose(fp);
	}
}

void fatal (char * errstr, modbusport *mp)
{
	err(errstr, mp);
	if (mp != NULL)
	{
		modbus_close(mp->port);
		modbus_free(mp->port);
	}
	err("exiting.", mp);
	exit(-1);
}

void log_init(modbusport *mp)
{
	snprintf(mp->logfile, sizeof(mp->logfile),
		"%s/%s-log.%ld", mp->logdir, mp->dname, mp->pid);

	snprintf(mp->errfile, sizeof(mp->errfile),
		"%s/%s-err.%ld", mp->logdir, mp->dname, mp->pid);
}

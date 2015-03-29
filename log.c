#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include "init.h"
#include "time.h"

void err (char *errstr, modbusport *mp)
{
	char msg[MAX_LOG_LEN];
	FILE *fp;

	snprintf(msg, sizeof(msg), "[%s][%s.%ld]:error: %s %s",
			timestamp(), mp->dname, mp->pid, errstr, strerror(errno));

	if ((fp = fopen(mp->errfile, "a")) == NULL)
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

	if ((fp = fopen(mp->logfile, "a")) == NULL)
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
	logger("exiting.", mp);
	exit(-1);
}

void log_init(modbusport *mp)
{
	char logfile[MAX_PATH_LEN];
	char errfile[MAX_PATH_LEN];
	mp->pid = (unsigned long) getpid();

	snprintf(mp->logfile, sizeof(mp->logfile),
		"%s/%s-log.%ld", LOGDIR, mp->dname, mp->pid);

	snprintf(mp->errfile, sizeof(mp->errfile),
		"%s/%s-err.%ld", LOGDIR, mp->dname, mp->pid);
}

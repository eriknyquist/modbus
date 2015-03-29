#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include "init.h"
#include "shared.h"
#include "time.h"

void err (char *errstr, modbusport *mp)
{
	char msg[MAX_LOG_LEN];
	char ts[TIMESTAMP_LEN];
	timestamp(ts);
	FILE *fp;

	snprintf(msg, sizeof(msg), "[%s][%s](%ld) error: %s %s",
			ts, mp->dname, mp->pid, errstr, strerror(errno));

	if (mp->errfp == NULL)
		fp = stderr;
	else
		fp = mp->errfp;

	fprintf(fp, "%s\n", msg);
}

void logger (char *str, modbusport *mp)
{
	char msg[MAX_LOG_LEN];
	char ts[TIMESTAMP_LEN];
	timestamp(ts);
	FILE *fp;

	snprintf(msg, sizeof(msg), "[%s][%s](%ld) log: %s",
			ts, mp->dname, mp->pid, str);

	if (mp->logfp == NULL)
		fp = stdout;
	else
		fp = mp->logfp;

	fprintf(fp, "%s\n", msg);
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

	snprintf(logfile, sizeof(logfile),
		"%s/%s-log.%ld", LOGDIR, mp->dname, mp->pid);

	snprintf(errfile, sizeof(errfile),
		"%s/%s-err.%ld", LOGDIR, mp->dname, mp->pid);

	if ((mp->logfp = fopen(logfile, "w+")) == NULL)
		err("can't open logfile in " LOGDIR " for writing", mp);

	if ((mp->errfp = fopen(errfile, "w+")) == NULL)
		err("can't open error file in " LOGDIR " for writing", mp);
}

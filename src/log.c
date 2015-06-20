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
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "init.h"
#include "time.h"

void err (char *errstr, logging *lp, mbdinfo *mip)
{
	char msg[MAX_LOG_LEN];
	FILE *fp;

	snprintf(msg, sizeof(msg), "[%s][%s.%ld] error: %s %s",
			timestamp(), mip->dname, mip->pid, errstr, strerror(errno));

	/* if log location not defined or inaccessible, print to stderr */
	if (strlen(lp->logdir) == 0 || (fp = fopen(lp->errfile, "a")) == NULL) {
		fprintf(stderr, "%s\n", msg);
	} else {
		fprintf(fp, "%s\n", msg);	
		fclose(fp);
	}
}

void logger (char *str, logging *lp, mbdinfo *mip)
{
	char msg[MAX_LOG_LEN];
	FILE *fp;

	/* if log location not defined or inaccessible, print to stdout */
	snprintf(msg, sizeof(msg), "[%s][%s.%ld] log: %s",
			timestamp(), mip->dname, mip->pid, str);

	if (strlen(lp->logdir) == 0 || (fp = fopen(lp->logfile, "a")) == NULL) {
		fprintf(stdout, "%s\n", msg);
	} else {
		fprintf(fp, "%s\n", msg);	
		fclose(fp);
	}
}

void fatal (char *errstr, modbusport *mp, logging *lp, mbdinfo *mip, int er)
{
	err(errstr, lp, mip);

	if (mp != NULL) {
		modbus_close(mp->port);
		modbus_free(mp->port);
	}

	err("exiting.", lp, mip);
	exit(er);
}

void log_init(logging *lp, mbdinfo *mip)
{
	/* if log location has been defined in conf file, create logfile names
	 * based on executable name & PID */
	if (strlen(lp->logdir) > 0) {
		snprintf(lp->logfile, sizeof(lp->logfile),
			"%s/%s-log.%ld", lp->logdir, mip->dname, mip->pid);

		snprintf(lp->errfile, sizeof(lp->errfile),
			"%s/%s-err.%ld", lp->logdir, mip->dname, mip->pid);
	}
}

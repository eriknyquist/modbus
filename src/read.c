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
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "init.h"
#include "time.h"
#include "log.h"
#include "shared.h"

#define SENSOR_READING_HEADER   "<D>,SEC:PUBLIC"
#define CLRLINE                 "\033[1A\033[2K"

void update_monitor(mbdport *mp, mbdinfo *mip, element *pv)
{
	int i;

	if (mp->rticks > 0) {
		for (i = 0; i < mp->read_count + 4; i++)
			printf(CLRLINE);	
	} else {
		printf("\n");
		printf("%-18s%-30s%-18s%-8s\n", "ID", "Tag",
		       "Raw value", "Scaled value");
	}

	printf("\n");
	for (i = 0; i < mp->read_count; i++) {
		printf("%-18s%-30s%-1s%-18x%-8.2f\n", pv[i].id,
		       pv[i].tag, "0x", pv[i].value_raw,
		       pv[i].value_scaled);
	}

	printf("\nread number : %lld\n\n", mp->rticks);
}

/* mbd_read */
int mbd_read (mbdport *mp, element *pv, logging *lp, mbdinfo *mip)
{
	int ret;
	int i;
	int n = 1;

	ret = 0;
#ifndef NOMODBUS

	/* try to perform a read */
	pthread_mutex_lock(&mp->lock);
	n = modbus_read_registers(mp->port, mp->read_base, mp->read_count,
	                          mp->inputs_raw);
	pthread_mutex_unlock(&mp->lock);

	if (n <= 0) {

		/* save the error state */
		int saved_err = errno;

		/* log the failure and start incrementing the retry counter */
		if (mp->maxretries < 0 || mp->retries <= mp->maxretries) {
			char msg[80];

			snprintf(msg, sizeof(msg), mp->retries > 0 ?
			         "retrying" : "reading modbus registers "
			         "failed");

			/* if the number of retries is set to 'infinity', only
			 * increment the retry counter once, and no more. */
			if (mp->maxretries >= 0 || mp->retries == 0)
				mp->retries++;

			err(msg, lp, mip, saved_err);
			ret = 1;
		} else {

			/* retries finished- abort. */
			fatal("Unable to read modbus registers", mp, lp, mip,
			      saved_err);
		}
	} else {
		if (mp->retries != 0)

			/* recovery- reset the
			 * retry counter */
			mp->retries = 0;
	}
#endif

	for (i = 0; i < mp->read_count; i++) {
		pv[i].value_raw = mp->inputs_raw[i];
		pv[i].value_scaled = (float) mp->inputs_raw[i] * pv[i].scale;
	}

	if (mip->monitor && n > 0)
		/* monitor mode: pretty-print modbus register data */
		update_monitor(mp, mip, pv);

	mp->rticks++;

	return ret;
}

int posmatch (int maj, int min, int read_count, element *pv)
{
	int i;
	for (i = 0; i < read_count; i++) {
		if (pv[i].major == maj && pv[i].minor == min)
			return i;
	}

	return -1;
}

void mbd_write_sensordata(mbdport *mp, element *pv, logging *lp, mbdinfo *mip)
{
	FILE *fp;
	int i;
	int j;
	int count;
	int saved_err;
	char *logfilename;
	char logpath[MAX_PATH_LEN];
	size_t ldelta;

	logfilename = gen_filename(mip->uuid);
	snprintf(logpath, sizeof(logpath), "%s/%s", lp->sens_logdir,
	         logfilename);

	if (lp->verbosity == LOG_VERBOSE)
		logger("writing to sensor log directory", lp, mip);

	if ((fp = fopen(logpath, "w")) == NULL) {
		saved_err = errno;

		err("can't open sensor log directory to write data", lp, mip,
		    saved_err);
	}

	for (j = 0; j < mp->read_count; j++) {
		int ix = posmatch(j, 0, mp->read_count, pv);

		if (ix == -1)
			continue;

		char line[MAX_SENS_LOGLEN];

		strncpy(line, SENSOR_READING_HEADER, sizeof(line));
		if ((int) sizeof(line) > 0)
			line[MAX_SENS_LOGLEN - 1] = '\0';		

		for (i = 0; i < mp->read_count; i++) {
			char buf[MAX_SENS_RLEN];

			ix = posmatch(j, i, mp->read_count, pv);

			if (ix == -1)
				continue;

			count = snprintf(buf, sizeof(buf), ",%s:%.2f",
				pv[ix].tag, pv[ix].value_scaled);
			if ((int) sizeof(buf) <= count)
				buf[((int) sizeof(buf)) - 1] = '\0';

			ldelta = sizeof(line) - strlen(line);

			strncat(line, buf, ldelta - 1);
		}

		fputs(line, fp);
		fputc('\n', fp);
	}
	fclose(fp);
	free(logfilename);
}

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
#include "init.h"
#include "time.h"
#include "log.h"
#include "shared.h"

#define SENSOR_READING_HEADER   "<D>,SEC:PUBLIC"
#define CLRLINE                 "\033[1A\033[2K"

/* TODO: put this ptr inside modbusport struct */
extern uint16_t *inputs_raw;

void mbd_read (modbusport *mp, element *pv, logging *lp, mbdinfo *mip)
{
	int i;
#ifndef NOMODBUS
	int n;

	if (lp->verbosity == LOG_VERBOSE) {
		char msg[MAX_LOG_LEN];
		snprintf(msg, sizeof(msg), "reading modbus registers %d to %d from '%s'",
			mp->read_base, mp->read_base + mp->read_count, mp->port_name);
		logger(msg, lp, mip);
	}

	n = modbus_read_registers(mp->port, mp->read_base, mp->read_count, inputs_raw);

	if (n <= 0)
		fatal("Unable to read modbus registers", mp, lp, mip, errno);
#endif

	for (i = 0; i < mp->read_count; i++) {
		pv[i].value_raw = inputs_raw[i];
		pv[i].value_scaled = (float) inputs_raw[i] * pv[i].scale;
	}

#ifdef DEBUG
	if (mp->readcount > 0) {
		for (i = 0; i < mp->read_count + 4; i++)
			printf(CLRLINE);	
	} else {
		printf("\n");
	}

	printf("\n");
	for (i = 0; i < mp->read_count; i++) {
		printf("%24s : %.2f\n", pv[i].id, pv[i].value_scaled);
	}

	printf("\nread number : %lld\n\n", mp->readcount);
#endif

	mp->readcount++;	
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

void write_registers_tofile(modbusport *mp, element *pv, logging *lp, mbdinfo *mip)
{
	FILE *fp;
	int i, j;
	char *logfilename;
	char logpath[MAX_PATH_LEN];

	logfilename = gen_filename(mip->uuid);
	snprintf(logpath, sizeof(logpath), "%s/%s", lp->sens_logdir, logfilename);

	if (lp->verbosity == LOG_VERBOSE)
		logger("writing to sensor log directory", lp, mip);

	if ((fp = fopen(logpath, "w")) == NULL)
		err("can't open sensor log directory to write data", lp, mip);

	for (j = 0; j < mp->read_count; j++) {
		int ix = posmatch(j, 0, mp->read_count, pv);

		if (ix == -1)
			continue;

		char outstring[512];
		char buf[80];
		strcpy(outstring, SENSOR_READING_HEADER);

		for (i = 0; i < mp->read_count; i++) {
			ix = posmatch(j, i, mp->read_count, pv);

			if (ix == -1)
				continue;

			snprintf(buf, sizeof(buf), ",%s:%.2f",
				pv[ix].tag, pv[ix].value_scaled);
			strcat(outstring, buf);
		}

		fputs(outstring, fp);
		fputc('\n', fp);
	}
	fclose(fp);
	free(logfilename);
}

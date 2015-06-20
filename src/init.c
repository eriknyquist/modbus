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
#include <sys/types.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "init.h"
#include "confparse.h"
#include "shared.h"
#include "read.h"
#include "time.h"
#include "log.h"

#define MB_DATABITS       8
#define MB_STOPBITS       2
#define MB_PARITY         'N'

int paramcount;
int er;

double delaytime;
static FILE *fp = NULL;

uint16_t *inputs_raw;

void get_modbus_params(modbusport *mp, logging *lp, mbdinfo *mip)
{
        if (access(mip->conffile, F_OK) != 0 && lp->verbosity != LOG_QUIET) {
		logger("Configuration file inaccessible. Using defaults.", lp, mip);
        } else {
                if ((fp = fopen(mip->conffile, "r")) == NULL) {
			if (lp->verbosity != LOG_QUIET) {
                        	fprintf(stderr, "Error opening '%s' for "
				                "reading:\n%s\n", mip->conffile,
				                strerror(errno));
			}

                        exit(ENOENT);
                }

		/* parse 1st section of conf file, i.e. modbus parameters */
                parse_modbus_params(fp, mp, lp, mip);
        }
}

void modbus_init (modbusport *mp, element *pv, logging *lp, mbdinfo *mip)
{
	int i;

	for (i = 0; i < mp->read_count; i++) {
		if (fp == NULL || feof(fp) || get_next_regparam(fp, &(pv[i])) == EOF) {
			snprintf(pv[i].tag, sizeof(pv[i].tag), "SENS_REG_%d", i + mp->read_base);
			snprintf(pv[i].id, sizeof(pv[i].id), "reg%d", i + mp->read_base);
			pv[i].scale = 1;
			pv[i].major = 0;
			pv[i].minor = i;
		} else {
			pv[i].major = -1;
			pv[i].minor = -1;
		}
	}

	if (!feof(fp)) {
		/* figure out the order in which readings should be
  		 * arranged in the logging of register data */
		parse_order(fp, pv, mp);
	}

	fclose(fp);

	if (lp->verbosity == LOG_VERBOSE) {
		/* report settings to the daemon's logfile */
		for (i = 0; i < mp->read_count; i++) {
			int msglen = 30 + strlen(pv[i].id) + strlen(pv[i].tag);
			char msg[msglen];
			snprintf(msg, msglen, "register #%d; id=\"%s\", tag=\"%s\"",
			         mp->read_base + i, pv[i].id, pv[i].tag);
			logger(msg, lp, mip);
		}
	}

	mp->readcount = 0;
	
/* NOMODBUS macro is handy for debugging conf file parsing & periodic 
 * timer on a system with no modbus slave- most development is done
 * on a VM. Remove before submission. */
#ifndef NOMODBUS

	/* configure modbus port settings & create modbus context */

	if (access(mp->port_name, F_OK) != 0) {
		er = errno;
		if (lp->verbosity != LOG_QUIET) {
			fprintf(stderr, "Error accessing '%s':\n%s\n",
		                        mp->port_name, strerror(errno));
		}

		exit(er);
	}

	mp->port = modbus_new_rtu(mp->port_name, mp->rtu_baud, MB_PARITY,
	                          MB_DATABITS, MB_STOPBITS);

	if (mp->port == NULL) {
		er = errno;
		if (lp->verbosity != LOG_QUIET) {
			fprintf(stderr, "Unable to create the libmodbus "
			                "context on serial port %s\n%s\n",
		                        mp->port_name, strerror(errno));
		}

		exit(er);
	}

	if (modbus_set_slave(mp->port, mp->station_id)) {
		er = errno;
		if (lp->verbosity != LOG_QUIET)
			fprintf(stderr, "Failed to set modbus slave address");

		exit(er);
	}

	if (modbus_connect(mp->port)) {
		er = errno;
		if (lp->verbosity != LOG_QUIET)
			fprintf(stderr, "Unable to connect to modbus server");

		exit(er);
	}
#endif
}

void ile_aip_init(logging *lp, mbdinfo *mip)
{
	FILE *fp;

	if ((fp = fopen(mip->uuidfile, "r")) == NULL) {
		fprintf(stderr, "Failed to open UUID file %s:\n%s\n",
		                mip->uuidfile, strerror(errno));
		exit(errno);
	}

	fgets(mip->uuid, UUID_LENGTH, fp);

	if (strlen(mip->uuid) != UUID_LENGTH - 1) {
		fprintf(stderr, "Error : file '%s' does not contain a UUID in "
		                "the expected format\n%s\n%zu\n", mip->uuidfile,
		                mip->uuid, strlen(mip->uuid));
		exit(EINVAL);
	}

	if (access(lp->sens_logdir, F_OK) != 0) {
		fprintf(stderr, "Error accessing '%s':\n%s\n", lp->sens_logdir,
		                strerror(errno));
		exit(errno);
	}
}

element *mbd_init(modbusport *mp, logging *lp, mbdinfo *mip)
{
	element *p;

	/* initialisation for modbus register data logging */
	ile_aip_init(lp, mip);
	
	/* read the modbus params from conf file, so we know
 	 * how many modbus registers we're reading */
	get_modbus_params(mp, lp, mip);

	/* initialisation for daemon logging */
	if (lp->verbosity != LOG_QUIET)
		log_init(lp, mip);

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(mp->read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, mp->read_count * sizeof(uint16_t));

	/* allocate space to store scaled register reads, along 
 	 * with additional information (IDs, tags etc) */
	p = malloc(sizeof(element) * mp->read_count);

	/* read the remainder of conf file, if any, and initiate
 	 * modbus connection */
	modbus_init(mp, p, lp, mip);
	return p;
}

void mbd_exit(modbusport *mp, logging *lp, mbdinfo *mip)
{
	if (lp->verbosity != LOG_QUIET)
		logger("killed. Closing modbus connection & exiting.", lp, mip);
	modbus_close(mp->port);
	modbus_free(mp->port);
	free(inputs_raw);
}

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
#include "parse.h"
#include "read.h"
#include "time.h"
#include "log.h"

#define INTERVAL_MIN      1
#define INTERVAL_MAX      3600
#define MB_DATABITS       8
#define MB_STOPBITS       2
#define MB_PARITY         'N'

int paramcount;
double delaytime;
static FILE *fp = NULL;

uint16_t *inputs_raw;

void get_modbus_params(modbusport *mp, logging *lp)
{
        if (access(CONF_FILE, F_OK) != 0) {
		logger("\"" CONF_FILE "\" no such file. Using defaults.", lp);
        } else {
                if ((fp = fopen(CONF_FILE, "r")) == NULL) {
                        fprintf(stderr, "'%s' for reading:\n%s\n",
                                CONF_FILE, strerror(errno));
                        exit(-1);
                }

		/* parse 1st section of conf file, i.e. modbus paramaters */
                parse_modbus_params(fp, mp, lp);
        }

        if (mp->secs < INTERVAL_MIN || mp->secs > INTERVAL_MAX) {
                fprintf(stderr, "Error in configuration file '%s' : parameter \n"
                        "'interval_secs' must be between %d and %d.\n"
                        "You have entered a value of %ld\n",
                        CONF_FILE, INTERVAL_MIN, INTERVAL_MAX, mp->secs);
                exit(-1);
        }

}

void modbus_init (modbusport *mp, element *pv, logging *lp)
{
	int i;
	int eof = 0;

	for (i = 0; i < mp->read_count; i++) {
		if (fp == NULL || eof || (eof = get_next_regparam(fp, &(pv[i])))) {
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

	if (!eof) {
		/* figure out the order in which readings should be
  		 * arranged in the logging of register data */
		parse_order(fp, pv, mp);
	}

	fclose(fp);

	/* report settings to the daemon's logfile */
	for (i = 0; i < mp->read_count; i++) {
		int msglen = 30 + strlen(pv[i].id) + strlen(pv[i].tag);
		char msg[msglen];
		snprintf(msg, msglen, "register #%d; id=\"%s\", tag=\"%s\"",
			mp->read_base + i, pv[i].id, pv[i].tag);
		logger(msg, lp);
	}

	mp->readcount = 0;
	
/* NOMODBUS macro is handy for debugging conf file parsing & periodic 
 * timer on a system with no modbus slave- most development is done
 * on a VM. Remove before submission. */
#ifndef NOMODBUS

	/* configure modbus port settings & create modbus context */

	if (access(mp->port_name, F_OK) != 0) {
		printf("Error accessing '%s':\n%s\n", mp->port_name, strerror(errno));
		exit(-1);
	}

	mp->port = modbus_new_rtu(mp->port_name, mp->rtu_baud, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (mp->port == NULL) {
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n%s\n",
			mp->port_name, 
			strerror(errno));
		exit(-1);
	}

	if (modbus_set_slave(mp->port, mp->station_id))
		fatal("Failed to set modbus slave address", mp, lp);

	if (modbus_connect(mp->port))
		fatal("Unable to connect to modbus server", mp, lp);
#endif
}

void ile_aip_init(logging *lp)
{
	FILE *fp;

	if ((fp = fopen(lp->uuidfile, "r")) == NULL) {
		fprintf(stderr, "Failed to open UUID file %s:\n%s\n",
			lp->uuidfile, strerror(errno));
			exit(errno);
	}

	fgets(lp->uuid, UUID_LENGTH, fp);

	if (strlen(lp->uuid) != UUID_LENGTH - 1) {
		fprintf(stderr, 
			"Error : file '%s' does not contain a UUID in the expected format\n%s\n%zu\n",
			lp->uuidfile, lp->uuid, strlen(lp->uuid));
		exit(-1);
	}

	if (access(lp->sens_logdir, F_OK) != 0) {
		fprintf(stderr, "Error accessing '%s':\n%s\n", lp->sens_logdir, strerror(errno));
		exit(errno);
	}
}

element *mbd_init(modbusport *mp, logging *lp)
{
	element *p;
	/* initialisation for modbus register data logging */
	ile_aip_init(lp);
	
	/* read the modbus params from conf file, so we know
 	 * how many modbus registers we're reading */
	get_modbus_params(mp, lp);
	/* initialisation for daemon logging */
	log_init(lp);

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(mp->read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, mp->read_count * sizeof(uint16_t));

	/* allocate space to store scaled register reads, along 
 	 * with additional information (IDs, tags etc) */
	p = malloc(sizeof(element) * mp->read_count);

	/* read the remainder of conf file, if any, and initiate
 	 * modbus connection */
	modbus_init(mp, p, lp);
	return p;
}

void mbd_exit(modbusport *mp, logging *lp)
{
	logger("killed. Closing modbus connection & exiting.", lp);
	modbus_close(mp->port);
	modbus_free(mp->port);
	free(inputs_raw);
}

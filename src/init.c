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
#include <sys/stat.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "init.h"
#include "control.h"
#include "confparse.h"
#include "shared.h"
#include "read.h"
#include "time.h"
#include "log.h"

#define MB_DATABITS       8
#define MB_STOPBITS       2
#define MB_PARITY         'N'

int paramcount;

double delaytime;
static FILE *fp;

int is_uuid (char c)
{
	return ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
	        (c >= '0' && c <= '9') || c == '-') ? 1 : 0;
}

void get_modbus_params(mbdport *mp, logging *lp, mbdinfo *mip)
{
	if (access(mip->conffile, F_OK) != 0 && lp->verbosity != LOG_QUIET) {
		logger("Configuration file inaccessible. Using defaults",
		       lp, mip);
	} else {
		if ((fp = fopen(mip->conffile, "r")) == NULL) {
			int saved_err = errno;

			if (lp->verbosity != LOG_QUIET) {
				fprintf(stderr, "Error opening '%s' for "
				        "reading:\n%s\n", mip->conffile,
				        strerror(saved_err));
			}

			exit(ENOENT);
		}

		/* parse 1st section of conf file, i.e. modbus parameters */
		if (parse_modbus_params(fp, mp, lp, mip) == EOF)
			logger("Configuration file contains no settings. "
			       "Using defaults", lp, mip);
	}
}

void modbus_init (mbdport *mp, element *pv, logging *lp, mbdinfo *mip)
{
	int i;

	for (i = 0; i < mp->read_count; i++) {
		if (fp == NULL || feof(fp) ||
		    get_next_regparam(fp, &(pv[i])) == EOF) {
			int count;

			count = snprintf(pv[i].tag, sizeof(pv[i].tag),
			        "SENS_REG_%d", i + mp->read_base);
			if ((int) sizeof(pv[i].tag) <= count)
				pv[i].tag[((int) sizeof(pv[i].tag)) - 1] = '\0';

			count = snprintf(pv[i].id, sizeof(pv[i].id), "reg%d",
			         i + mp->read_base);
			if ((int) sizeof(pv[i].id) <= count)
				pv[i].id[((int) sizeof(pv[i].id)) -1] = '\0';

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
			snprintf(msg, msglen,
			         "register #%d; id=\"%s\", tag=\"%s\"",
			         mp->read_base + i, pv[i].id, pv[i].tag);
			logger(msg, lp, mip);
		}
	}
	
/* NOMODBUS macro is handy for debugging conf file parsing & periodic 
 * timer on a system with no modbus slave- most development is done
 * on a VM. Remove before submission. */
#ifndef NOMODBUS

	int saved_err;

	/* configure modbus port settings & create modbus context */
	if (access(mp->port_name, F_OK) != 0) {
		saved_err = errno;
		if (lp->verbosity != LOG_QUIET) {
			fprintf(stderr, "Error accessing '%s':\n%s\n",
		                        mp->port_name, strerror(saved_err));
		}

		exit(saved_err);
	}

	mp->port = modbus_new_rtu(mp->port_name, mp->rtu_baud, MB_PARITY,
	                          MB_DATABITS, MB_STOPBITS);

	if (mp->port == NULL) {
		saved_err = errno;
		if (lp->verbosity != LOG_QUIET) {
			fprintf(stderr, "Unable to create the libmodbus "
			                "context on serial port %s\n%s\n",
		                        mp->port_name, strerror(saved_err));
		}

		exit(saved_err);
	}

	if (modbus_set_slave(mp->port, mp->station_id)) {
		saved_err = errno;
		if (lp->verbosity != LOG_QUIET)
			fprintf(stderr, "Failed to set modbus slave address");

		exit(saved_err);
	}

	if (modbus_connect(mp->port)) {
		saved_err = errno;
		if (lp->verbosity != LOG_QUIET)
			fprintf(stderr, "Unable to connect to modbus server");

		exit(saved_err);
	}
#endif

	mp->rticks = 0;
	mp->wticks = 0;

	pthread_mutex_init (&mp->lock, NULL);
}

void ile_aip_init(logging *lp, mbdinfo *mip)
{
	int i = 0;
	char c;
	int saved_err;
	FILE *fp;

	if (access(mip->uuidfile, F_OK) != 0) {
		saved_err = errno;
		printf("Can't access UUID file %s:\n%s\n", mip->uuidfile,
		       strerror(saved_err));
		exit(saved_err);
	}

	if ((fp = fopen(mip->uuidfile, "r")) == NULL) {
		saved_err = errno;
		printf("Failed to open UUID file %s:\n%s\n", mip->uuidfile,
		       strerror(saved_err));
		exit(saved_err);
	}


	while ((c = fgetc(fp)) != EOF && i < UUID_LENGTH)
		if (is_uuid(c) > 0)
			mip->uuid[i++] = c;

	mip->uuid[i] = '\0';

	fclose(fp);

	if (strlen(mip->uuid) != UUID_LENGTH) {
		fprintf(stderr, "Error : file '%s' does not contain a UUID in "
		                "the expected format\n%s\n(Looking for %d "
			        "characters, found %zu)\n", mip->uuidfile,
		                mip->uuid, UUID_LENGTH, strlen(mip->uuid));
		exit(EINVAL);
	}

	if (access(lp->sens_logdir, F_OK) != 0) {
		saved_err = errno;
		fprintf(stderr, "Error accessing '%s':\n%s\n", lp->sens_logdir,
		                strerror(saved_err));
		exit(saved_err);
	}
}

element *modbus_mem_alloc (mbdport *mp, logging *lp, mbdinfo *mip)
{
	int pos;
	int saved_err;
	size_t inputs_raw_size;
	size_t inputs_scaled_size;
	element *inputs_scaled;
	char msg[120];

	inputs_raw_size = mp->read_count * sizeof(uint16_t);
	inputs_scaled_size = mp->read_count * sizeof(element);

	/* Allocate space to store raw register reads */
	mp->inputs_raw = (uint16_t *) malloc(inputs_raw_size);

	if (mp->inputs_raw == NULL) {
		saved_err = errno;
		pos = snprintf(msg, sizeof(msg), "Can't allocate %zu bytes "
		               "for mapping modbus registers", inputs_raw_size);
		if (sizeof(msg) <= pos)
			msg[sizeof(msg) -1] = '\0';

		fatal(msg, mp, lp, mip, saved_err);
	}

	memset(mp->inputs_raw, 0, mp->read_count * sizeof(uint16_t));

	/* allocate space to store scaled register reads, along 
	 * with additional information (IDs, tags etc) */
	inputs_scaled = malloc(inputs_scaled_size);

	if (inputs_scaled == NULL) {
		saved_err = errno;
		pos = snprintf(msg, sizeof(msg), "Can't allocate %zu bytes for "
		         "scaled register reads", inputs_scaled_size);
		if ((int) sizeof(msg) <= pos)
			msg[((int) sizeof(msg)) - 1] = '\0';

		fatal(msg, mp, lp, mip, saved_err);
	}

	return inputs_scaled;
}

element *mbd_init(mbdport *mp, logging *lp, mbdinfo *mip)
{
	int ret;
	int pos;
	element *inputs_scaled;

	/* read the modbus params from conf file, so we know
	 * how many modbus registers we're reading */
	get_modbus_params(mp, lp, mip);

	/* initialisation for modbus register data logging */
	ile_aip_init(lp, mip);

	/* initialisation for daemon logging */
	if (!mip->monitor && lp->verbosity != LOG_QUIET)
		log_init(lp, mip);

	/* allocate memory for modbus address space */
	inputs_scaled = modbus_mem_alloc(mp, lp, mip);

	/* read the remainder of conf file, if any, and initiate
	 * modbus connection */
	modbus_init(mp, inputs_scaled, lp, mip);

	/* Enable drive for operation */
	ret = init_drive_ready(mp, mip, lp);
	if (ret != 0) {
		fatal("Error initialising drive to ready state", mp, lp, mip,
		      ret);
	}

	/* create FIFO for receiving control data */
	pos = snprintf(mip->fifo, sizeof(mip->fifo), "%s.%lu", CONTROL_FIFO_PATH,
	         mip->pid);
	if ((int) sizeof(mip->fifo) <= pos)
		mip->fifo[(int) sizeof(mip->fifo) - 1] = '\0';

	if (access(mip->fifo, F_OK) != 0) {
		mkfifo(mip->fifo, 0777);
	}

	return inputs_scaled;
}

void mbd_exit(mbdport *mp, logging *lp, mbdinfo *mip)
{
	if (lp->verbosity != LOG_QUIET)
		logger("killed. Closing modbus connection & exiting.", lp, mip);

	pthread_mutex_lock(&mp->lock);
	modbus_close(mp->port);
	modbus_free(mp->port);
	pthread_mutex_unlock(&mp->lock);

	free(mp->inputs_raw);
}

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

#define UUID_FILE "/uuid"
#define INTERVAL_MIN 1
#define INTERVAL_MAX 3600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'

int paramcount;
double delaytime;
static FILE *fp = NULL;

uint16_t *inputs_raw;

void get_modbus_params(modbusport *mp)
{
        if (access(CONF_FILE, F_OK) != 0) {
		logger("\"" CONF_FILE "\" no such file. Using defaults.", mp);
        } else {
                if ((fp = fopen(CONF_FILE, "r")) == NULL) {
                        fprintf(stderr, "'%s' for reading:\n%s\n",
                                CONF_FILE, strerror(errno));
                        exit(-1);
                }

		/* parse 1st section of conf file, i.e. modbus paramaters */
                parse_modbus_params(fp, mp);
        }

        if (mp->secs < INTERVAL_MIN || mp->secs > INTERVAL_MAX) {
                fprintf(stderr, "Error in configuration file '%s' : parameter \n"
                        "'interval_secs' must be between %d and %d.\n"
                        "You have entered a value of %ld\n",
                        CONF_FILE, INTERVAL_MIN, INTERVAL_MAX, mp->secs);
                exit(-1);
        }

}

void modbus_init (modbusport *mp, element *pv)
{
	int i;
	int eof = 0;

	for (i = 0; i < mp->read_count; i++) {
		if (fp == NULL || eof || (eof = get_next_regparam(fp, &(pv[i])))) {
			/* if this register does not have a tag or scale factor 
  			 * defined, use defaults. TODO: make default tags and IDs
  			 * unique i.e. default_id_1, default_id_2 */
			strncpy(pv[i].tag, "SENS_DEFAULT", sizeof(pv[i].tag));
			strncpy(pv[i].id, "default_id", sizeof(pv[i].id));
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
		logger(msg, mp);
	}

	
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
		fatal("Failed to set modbus slave address", mp);

	if (modbus_connect(mp->port))
		fatal("Unable to connect to modbus server", mp);
#endif

/* DEBUG macro useful for printing register reads to stdout. 
 * Remove before submission. */
#ifdef DEBUG
	printf("\n");
	for (i = 0; i < mp->read_count; i++) {
		printf("%16s", pv[i].id);
	}

	printf("\n");
#endif
}

void ile_aip_init(modbusport *mp)
{
	FILE *fp;

	if ((fp = fopen(UUID_FILE, "r")) == NULL) {
		fprintf(stderr, "Failed to open UUID file %s:\n%s\n",
			UUID_FILE, strerror(errno));
			exit(errno);
	}

	fgets(mp->uuid, UUID_LENGTH, fp);

	if (strlen(mp->uuid) != UUID_LENGTH - 1) {
		fprintf(stderr, 
			"Error : file '%s' does not contain a UUID in the expected format\n%s\n%zu\n",
			UUID_FILE, mp->uuid, strlen(mp->uuid));
		exit(-1);
	}

	if (access(SENSORDATA, F_OK) != 0) {
		fprintf(stderr, "Error accessing '%s':\n%s\n", SENSORDATA, strerror(errno));
		exit(errno);
	}
}

element *mbd_init(modbusport *mp)
{
	element *p;
	/* initialisation for modbus register data logging */
	ile_aip_init(mp);
	
	/* read the modbus params from conf file, so we know
 	 * how many modbus registers we're reading */
	get_modbus_params(mp);
	/* initialisation for daemon logging */
	log_init(mp);

	/* Allocate space to store raw register reads */
	inputs_raw = (uint16_t *) malloc(mp->read_count * sizeof(uint16_t));
	memset(inputs_raw, 0, mp->read_count * sizeof(uint16_t));

	/* allocate space to store scaled register reads, along 
 	 * with additional information (IDs, tags etc) */
	p = malloc(sizeof(element) * mp->read_count);

	/* read the remainder of conf file, if any, and initiate
 	 * modbus connection */
	modbus_init(mp, p);
	return p;
}

void mbd_exit(modbusport *mp)
{
	logger("killed. Closing modbus connection & exiting.", mp);
	modbus_close(mp->port);
	modbus_free(mp->port);
	free(inputs_raw);
}

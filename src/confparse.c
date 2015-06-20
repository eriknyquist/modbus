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
#include <limits.h>
#include "init.h"
#include "shared.h"
#include "time.h"
#include "log.h"

#define CONF_ID_BAUD            "modbus_rtu_baud"
#define CONF_ID_STATION_ID      "modbus_station_id"
#define CONF_ID_READ_BASE       "modbus_read_base"
#define CONF_ID_READ_COUNT      "modbus_read_count"
#define CONF_ID_INTERVAL        "interval_msecs"
#define CONF_ID_MODBUS_PORT     "modbus_port"
#define CONF_ID_LOGPATH         "log_directory"
#define CONF_ID_UUIDPATH        "uuid_path"
#define CONF_ID_SENSORLOGPATH   "sensor_log_directory"
#define CONF_ID_TAG             "tag"
#define CONF_ID_SCALE           "scale"

#define MAX_PARAM_LEN           80
#define MAX_ID_LEN              80
#define MAX_NUM_LEN             10
#define INTERVAL_MSECS_MIN      10

int paramcount;
double delaytime;
char uuid[38];

static int line = 1;
static int column = 1;

uint8_t is_id (char c)
{
	return ((c <= 'z'&& c >= 'a') || (c <= 'Z' && c >= 'A' ) ||
		c == '_') ? 1 : 0;
}

uint8_t is_dec (char c)
{
	return ((c <= '9' && c >= '0') || (c == '.')) ? 1 : 0 ;
}

uint8_t is_whitespace (char c)
{
	return (c == ' ' || c == '\t' || c == '\r' ||
		c == '\v' || c == '\n') ? 1 : 0;
}

void syntaxerr (char c)
{
	fprintf(stderr, "Syntax error '%c' in configuration file, line %d, column %d\n",
		c, line, column);
	exit(EINVAL);
}

void unrec_id(char *id)
{
	fprintf(stderr,
		"Unrecognised identifier '%s' in configuration file, line %d\n",
		id, line);
	exit(EINVAL);
}

void nosuchparam(char *param)
{
	fprintf(stderr, "Error in configuration file, line %d\n", line);
	fprintf(stderr, "'%s' : no such parameter.\n", param);
	exit(EINVAL);
}

void doubleassn (char *id)
{
	fprintf(stderr, "Error in configuration file, line %d\n", line);
	fprintf(stderr, "ID '%s' has already been assigned a position.\n", id);
	fprintf(stderr, "You cannot assign two positions to the same ID.\n");
	exit(EINVAL);
}

void convert_and_assign_msecs(modbusport *mp, char *value)
{
	errno = 0;
	mp->msecs = strtoul(value, NULL, 10);

	if (errno != 0) {
		fprintf(stderr, "%s : Please enter a number between %d and %lu\n",
			CONF_ID_INTERVAL,
			INTERVAL_MSECS_MIN,
			ULONG_MAX);
		exit(errno);
	}	
}

void assign (char *param, char *value, modbusport *mp, logging *lp,
             mbdinfo *mip)
{
	if (strcmp(param, CONF_ID_BAUD) == 0) {
		mp->rtu_baud = atoi(value);
	} else if (strcmp(param, CONF_ID_STATION_ID) == 0) {
		mp->station_id = atoi(value);
	} else if (strcmp(param, CONF_ID_READ_BASE) == 0) {
		mp->read_base = atoi(value);
	} else if (strcmp(param, CONF_ID_READ_COUNT) == 0) {
		mp->read_count = atoi(value);
	} else if (strcmp(param, CONF_ID_INTERVAL) == 0) {
		convert_and_assign_msecs(mp, value);		
	} else if (strcmp(param, CONF_ID_MODBUS_PORT) == 0) {
		strncpy(mp->port_name, value, sizeof(mp->port_name));
	} else if (strcmp(param, CONF_ID_LOGPATH) == 0) {
		strncpy(lp->logdir, value, sizeof(lp->logdir));
	} else if (strcmp(param, CONF_ID_UUIDPATH) == 0) {
		strncpy(mip->uuidfile, value, sizeof(mip->uuidfile));
	} else if (strcmp(param, CONF_ID_SENSORLOGPATH) == 0) {
		strncpy(lp->sens_logdir, value, sizeof(lp->sens_logdir));
	} else {
		nosuchparam(param);
	}

	paramcount++;

	if (lp->verbosity == LOG_VERBOSE) {
		char msg[MAX_LOG_LEN];
		snprintf(msg, sizeof(msg), "%s set to '%s'", param, value);
		logger(msg, lp, mip);
	}
}

int get_next_regparam(FILE *fp, element *e)
{
	int state = 0;
	char pbuf[MAX_PARAM_LEN];
	char scale[MAX_NUM_LEN];
	char c;	
	int idbufpos = 0;
	int pbufpos = 0;
	int tagpos = 0;
	int scalepos = 0;

	while (state != 6) {

		if ((c = fgetc(fp)) == EOF )
			return EOF;

		if (c == '\n') {
			line++;
			column = 1;
		}

		switch (state) {
		case 0:
			if (is_id(c)) {
				e->id[idbufpos] = c;
				idbufpos++;
				state = 1;
			} else if (c == '#') {
				state = 5;
			} else if (! is_whitespace(c)) {
				syntaxerr(c);
			}

			break;
		case 1:
			if (is_id(c)) {
				e->id[idbufpos] = c;
				idbufpos++;
			} else if (c == '{') {
				e->id[idbufpos] = '\0';
				state = 2;
			} else if (! is_whitespace(c)) {
				syntaxerr(c);
			}

			break;
		case 2:
			if (is_id(c)) {
				pbuf[pbufpos] = c;
				pbufpos++;
			} else if (c == '=') {
				pbuf[pbufpos] = '\0';

				if (strcmp(pbuf, CONF_ID_TAG) == 0)
					state = 3;
				else if (strcmp(pbuf, CONF_ID_SCALE) == 0)
					state = 4;
				else
					nosuchparam(pbuf);
				pbufpos = 0;
			} else if (! is_whitespace(c)) {
				syntaxerr(c);
			}

			break;
		case 3:
			if (is_id(c)) {
				e->tag[tagpos] = c;
					tagpos++;
			} else if (c == ',') {
				e->tag[tagpos] = '\0';
				state = 2;
			} else if (c == '}') {
				e->tag[tagpos] = '\0';
				state = 6;
			} else if (! is_whitespace(c)) {
					syntaxerr(c);
			}

			break;
		case 4:
			if (is_dec(c)) {
				scale[scalepos] = c;
				scalepos++;
			} else if (c == ',') {
				scale[scalepos] = '\0';
				state = 2; 
			} else if (c == '}') {
				scale[scalepos] = '\0';
				state = 6;
			}

			break;
		case 5:
			if (c == '\n')
				state = 0;
			break;
		}
		column++;
	}
	e->scale = atof(scale);

	return 0;
}

int idcmp (char *idc, element *v, modbusport *mp)
{
	int i;

	for (i = 0; i < mp->read_count; i++) {
		if (strcmp(idc, v[i].id) == 0)
			return i;
	}

	return -1;
}

void parse_order (FILE *fp, element *v, modbusport *mp)
{
	int majc = 0;
	int minc = 0;
	char idbuf[MAX_ID_LEN];
	char c;
	int idbufpos = 0;
	uint8_t state = 0;

	while ((c = fgetc(fp)) != EOF) {
		if (c == '\n') {
			line++;
			column = 1;
		}

		switch (state) {
		case 0:
			if (c == '{')
				state = 1;
			else if (c == '#')
				state = 2;
			else if (! is_whitespace(c))
				syntaxerr(c);

			break;
		case 1:
			if (is_id(c)) {
				idbuf[idbufpos] = c;
				idbufpos++;
			} else if (c == ',' || c == '}') {
				idbuf[idbufpos] = '\0';
				int ti = idcmp(idbuf, v, mp);

				if (ti == -1)
					unrec_id(idbuf);
				if (v[ti].major != -1)
					doubleassn(idbuf);

				v[ti].major = majc;
				v[ti].minor = minc;
				idbufpos = 0;

				if (c == '}') {
					majc++;
					minc = 0;
					state = 0;
				} else {
					minc++;
				}
			} else if (! is_whitespace(c)) {
				syntaxerr(c);
			}

			break;
		case 2:
			if (c == '\n')
				state = 0;

			break;
		}
		column++;
	}
}

void parse_modbus_params(FILE *fp, modbusport *mp, logging *lp, mbdinfo *mip)
{
	char c;
	uint8_t state = 0;
	uint8_t idbufpos = 0;
	uint8_t valbufpos = 0;
	char idbuf[MAX_PARAM_LEN];
	char valbuf[MAX_PATH_LEN];

	c = fgetc(fp);

	while (c != ';') {
		if (c == '\n') {
			line++;
			column = 1;
		}

		switch (state) {
		case 0:
			if (is_id(c)) {
				idbuf[idbufpos] = c;
				idbufpos++;
				state = 1;
			} else if (c == '#') {
				state = 3;
			} else if (! is_whitespace(c)) {
				syntaxerr(c);
			}

			break;
		case 1:
			if (is_id(c)) {
				idbuf[idbufpos] = c;
				idbufpos++;
			} else if (c == '=') {
				idbuf[idbufpos] = '\0';
				state = (strcmp(idbuf, CONF_ID_MODBUS_PORT) == 0
					|| strcmp(idbuf, CONF_ID_LOGPATH) == 0
					|| strcmp(idbuf, CONF_ID_UUIDPATH) == 0
					|| strcmp(idbuf, CONF_ID_SENSORLOGPATH) == 0)
					? 4 : 2;
			} else {
				syntaxerr(c);
			}

			break;
		case 2:
			if (is_dec(c)) {
				valbuf[valbufpos] = c;
				valbufpos++;
			} else if (c == ',') {
				valbuf[valbufpos] = '\0';
				assign(idbuf, valbuf, mp, lp, mip);
				state = 0;
				idbufpos = 0;
				valbufpos = 0;
			} else if (! is_whitespace(c)) {
				syntaxerr(c);
			}

			break;
		case 3:
			if (c == '\n')
				state = 0;
			break;
		case 4:
			if (c == ',') {
				valbuf[valbufpos] = '\0';
				assign(idbuf, valbuf, mp, lp, mip);
				state = 0;
				idbufpos = 0;
				valbufpos = 0;
			} else if (! is_whitespace(c)) {
				valbuf[valbufpos] = c;
				valbufpos++;
			}

			break;
		}
		column++;
		c = fgetc(fp);
	}
	valbuf[valbufpos] = '\0';
	assign(idbuf, valbuf, mp, lp, mip);
}

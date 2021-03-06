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
#define CONF_ID_RETRIES         "retries"
#define CONF_ID_INTERVAL        "interval_msecs"
#define CONF_ID_MODBUS_PORT     "modbus_port"
#define CONF_ID_LOGPATH         "log_directory"
#define CONF_ID_UUIDPATH        "uuid_path"
#define CONF_ID_SENSORLOGPATH   "sensor_log_directory"
#define CONF_ID_TAG             "tag"
#define CONF_ID_SCALE           "scale"
#define CONF_RETRIES_INFINITY   "infinity"

#define MAX_PARAM_LEN           80
#define MAX_ID_LEN              80
#define MAX_NUM_LEN             10
#define INTERVAL_MSECS_MIN      10

int paramcount;
double delaytime;
char uuid[38];

static int line = 1;
static int column = 1;

int is_id (char c)
{
	return ((c <= 'z'&& c >= 'a') || (c <= 'Z' && c >= 'A' ) ||
		c == '_') ? 1 : 0;
}

int is_dec (char c)
{
	return (c <= '9' && c >= '0') ? 1 : 0 ;
}

int is_whitespace (char c)
{
	return (c == ' ' || c == '\t' || c == '\r' ||
		c == '\v' || c == '\n') ? 1 : 0;
}

void syntaxerr (char c)
{
	fprintf(stderr,
	        "Syntax error '%c' in configuration file, line %d, column %d\n",
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

int only_has_digits(char *s)
{
	unsigned int len;
	unsigned int i;

	len = (unsigned int) strlen(s);

	for (i = 0; i < len; i++) {
		if (is_dec(s[i]) == 0)
			return 0;
	}

	return 1;
}

void convert_assign_ul(unsigned long *dest, char *source,
                       const char *conf_id, unsigned long min)
{
	int saved_err;
	errno = 0;
	*dest = strtoul(source, NULL, 10);

	if (only_has_digits(source) == 0 || errno != 0) {
		saved_err = errno;
		fprintf(stderr,
		        "%s : Please enter an unsigned integer between %lu "
		        "and %lu\n", conf_id, min, ULONG_MAX);
		exit(saved_err);
	}	
}

void convert_assign_uint(unsigned int *dest, char *source,
                             const char *conf_id, unsigned int min)
{
	int saved_err;
	errno = 0;
	*dest = (unsigned int) strtoul(source, NULL, 10);

	if (only_has_digits(source) == 0 || errno != 0) {
		saved_err = errno;
		fprintf(stderr,
		        "%s : Please enter an unsigned integer between %u "
		        "and %u\n", conf_id, min, UINT_MAX);
		exit(saved_err);
	}
}

void convert_assign_retries(int *dest, char *source)
{
	int saved_err;

	if (strcmp(source, CONF_RETRIES_INFINITY) == 0) {
		*dest = -1;
	} else {
		errno = 0;
		*dest = (int) strtol(source, NULL, 10);

		if (only_has_digits(source) == 0 || errno != 0) {
			saved_err = errno;
			fprintf(stderr,
			        "%s : Please enter an unsigned integer between"
			        " %d and %d, or '%s'\n", CONF_ID_RETRIES, 0,
			        INT_MAX, CONF_RETRIES_INFINITY);
			exit(saved_err);
		}
	}
}

void assign (char *param, char *value, mbdport *mp, logging *lp,
             mbdinfo *mip)
{
	if (strcmp(param, CONF_ID_BAUD) == 0) {
		convert_assign_uint(&mp->rtu_baud, value, CONF_ID_BAUD, 300);
	} else if (strcmp(param, CONF_ID_STATION_ID) == 0) {
		convert_assign_uint(&mp->station_id, value, CONF_ID_STATION_ID, 0);
	} else if (strcmp(param, CONF_ID_READ_BASE) == 0) {
		convert_assign_uint(&mp->read_base, value, CONF_ID_READ_BASE, 0);
	} else if (strcmp(param, CONF_ID_READ_COUNT) == 0) {
		convert_assign_uint(&mp->read_count, value, CONF_ID_READ_COUNT, 1);
	} else if (strcmp(param, CONF_ID_INTERVAL) == 0) {
		convert_assign_ul(&mp->msecs, value, CONF_ID_INTERVAL, 10);
	} else if (strcmp(param, CONF_ID_RETRIES) == 0) {
		convert_assign_retries(&mp->maxretries, value);
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

	if (lp->verbosity != LOG_QUIET) {
		char msg[MAX_LOG_LEN];
		int count;

		count = snprintf(msg, MAX_LOG_LEN, "%s set to '%s'", param, value);
		if (MAX_LOG_LEN <= count)
			msg[MAX_LOG_LEN -1] = '\0';

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
			if (is_id(c) == 1) {
				e->id[idbufpos] = c;
				idbufpos++;
				state = 1;
			} else if (c == '#') {
				state = 5;
			} else if (is_whitespace(c) == 0) {
				syntaxerr(c);
			}

			break;
		case 1:
			if (is_id(c) == 1) {
				e->id[idbufpos] = c;
				idbufpos++;
			} else if (c == '{') {
				e->id[idbufpos] = '\0';
				state = 2;
			} else if (is_whitespace(c) == 0) {
				syntaxerr(c);
			}

			break;
		case 2:
			if (is_id(c) == 1) {
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
			} else if (is_whitespace(c) == 0) {
				syntaxerr(c);
			}

			break;
		case 3:
			if (is_id(c) == 1) {
				e->tag[tagpos] = c;
					tagpos++;
			} else if (c == ',') {
				e->tag[tagpos] = '\0';
				state = 2;
			} else if (c == '}') {
				e->tag[tagpos] = '\0';
				state = 6;
			} else if (is_whitespace(c) == 0) {
					syntaxerr(c);
			}

			break;
		case 4:
			if (is_dec(c) == 1 || c == '.') {
				scale[scalepos] = c;
				scalepos++;
			} else if (c == ',') {
				scale[scalepos] = '\0';
				state = 2; 
			} else if (c == '}') {
				scale[scalepos] = '\0';
				state = 6;
			} else if (is_whitespace(c) == 0) {
				syntaxerr(c);
			}

			break;
		case 5:
			if (c == '\n')
				state = 0;
			break;
		}
		column++;
	}

	e->scale = (float) atof(scale);

	return 0;
}

int idcmp (char *idc, element *v, mbdport *mp)
{
	unsigned int i;

	for (i = 0; i < mp->read_count; i++) {
		if (strcmp(idc, v[i].id) == 0)
			return (int) i;
	}

	return -1;
}

void parse_order (FILE *fp, element *v, mbdport *mp)
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
			else if (is_whitespace(c) == 0)
				syntaxerr(c);

			break;
		case 1:
			if (is_id(c) == 1) {
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

int parse_modbus_params(FILE *fp, mbdport *mp, logging *lp, mbdinfo *mip)
{
	char c;
	uint8_t state = 0;
	uint8_t idbufpos = 0;
	uint8_t valbufpos = 0;
	char idbuf[MAX_PARAM_LEN];
	char valbuf[MAX_PATH_LEN];

	if ((c = fgetc(fp)) == EOF)
		return EOF;

	while (c != ';' && c != EOF) {
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
				state = 2;
			} else if (is_whitespace(c) == 0) {
				syntaxerr(c);
			}

			break;
		case 2:
			if (c == ',') {
				valbuf[valbufpos] = '\0';
				assign(idbuf, valbuf, mp, lp, mip);
				state = 0;
				idbufpos = 0;
				valbufpos = 0;
			} else if (is_whitespace(c) == 0) {
				valbuf[valbufpos] = c;
				valbufpos++;
			}

			break;
		case 3:
			if (c == '\n')
				state = 0;
			break;
		}
		column++;
		c = fgetc(fp);
	}
	valbuf[valbufpos] = '\0';
	assign(idbuf, valbuf, mp, lp, mip);

	return 0;
}

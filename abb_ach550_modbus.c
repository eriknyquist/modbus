#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "abb_ach550_modbus.h"
#include "abb_ach550_parse.h"
#include "abb_ach550_time.h"

#define SENSORDATA "/home/sensordata/"
#define UUID_LENGTH 37
#define UUID_FILE "/uuid"
#define UPDATE_FREQ_MIN 0.01
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'

int paramcount;
double delaytime;
char uuid[38];

element *pv;

static int line = 1;
static int column = 1;

void fail (char * errstr, modbus_t *mp)
{
	fprintf(stderr, "\n%s\n%s\n", errstr, strerror(errno));
	if (mp != NULL)
	{
		modbus_close(mp);
		modbus_free(mp);
	}
	exit(-1);
}

modbus_t *abb_ach550_modbus_init (modbusport *mp)
{
	FILE *fp = NULL;
	if (access(CONF_FILE, F_OK) != 0)
	{
		printf("\nWARNING: configuration file %s not found, or\n"
			"insufficient permissions. Default settings will be used,\n"
			"but they are unlikely to work the way you expect. It is\n"
			"recommended that you create one- see sample file\n"
			"'abb.conf' included with the source files.\n\n", CONF_FILE);
	}
	else
	{
		if ((fp = fopen(CONF_FILE, "r")) == NULL)
		{
			fprintf(stderr, "'%s' for reading:\n%s\n",
				CONF_FILE, strerror(errno));
			exit(-1);
		}
		printf("\n");
		parse_modbus_params(fp, mp);
		printf("\n");
	}

	if (((int) floorf(mp->update_freq_hz)) * 1000 <
		((int) floorf(UPDATE_FREQ_MIN)) * 1000)
	{
		fprintf(stderr, "Error in configuration file '%s' : parameter \n"
			"'update_frequency_hz' must be set to %.2f or higher.\n"
			"You have entered a value of %ld\n",
			CONF_FILE, UPDATE_FREQ_MIN, mp->update_freq_hz);
		exit(-1);
	}


	int i;
	pv = malloc(sizeof(element) * mp->read_count);
	for (i = 0; i < mp->read_count; i++)
	{
		
		if (fp == NULL)
		{
			strncpy(pv[i].tag, "SENS_DEFAULT", sizeof(pv[i].tag));
			strncpy(pv[i].id, "default_id", sizeof(pv[i].id));
			pv[i].scale = 1;
			pv[i].major = 0;
			pv[i].minor = i;
		}
		else
		{
			pv[i] = get_next_regparam(fp);
			pv[i].major = -1;
			pv[i].minor = -1;
		}
	}
		
	if (fp != NULL)
	{
		parse_order(fp, pv, mp);
		fclose(fp);	
	}

	printf("%24s : TAG\n", "ID");
	printf("%28s\n", "-----");
	for (i = 0; i < mp->read_count; i++)
	{
		printf("%24s : %s\n", pv[i].id, pv[i].tag);
	}

	

#ifndef NOMODBUS
	if (access(mp->port_name, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", mp->port_name, strerror(errno));
		exit(-1);
	}

	mp->port = modbus_new_rtu(mp->port_name, mp->rtu_baud, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (mp->port == NULL)
	{
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n%s\n",
			mp->port_name, 
			strerror(errno));
		exit(-1);
	}

	if (modbus_set_slave(mp->port, mp->station_id))
		fail("Failed to set modbus slave address", mp->port);

	if (modbus_connect(mp->port))
		fail("Unable to connect to modbus server", mp->port);
#endif /* NOMODBUS */

	printf("\n");
	for (i = 0; i < mp->read_count; i++)
	{
		printf("%16s", pv[i].id);
	}
	printf("\n");
	return mp->port;
}

void ile_aip_init(void)
{
	FILE *fp;
	if ((fp = fopen(UUID_FILE, "r")) == NULL)
		fail("Failed to open UUID file", NULL);
	fgets(uuid, UUID_LENGTH, fp);
	if (strlen(uuid) != UUID_LENGTH - 1)
	{
		fprintf(stderr, 
			"Error : file '%s' does not contain a UUID in the expected format\n%s\n%d\n",
			UUID_FILE, uuid, strlen(uuid));
		exit(-1);
	}
	if (access(SENSORDATA, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", SENSORDATA, strerror(errno));
		exit(-1);
	}
}

int abb_ach550_read (uint16_t *inputs_raw, modbusport *mp)
{
	int n, i;

#ifndef NOMODBUS
	n = modbus_read_registers(mp->port, mp->read_base, mp->read_count, inputs_raw);
	if (n <= 0)
	{
		fail("Unable to read modbus registers", mp->port);
	}
#endif

	for (i = 0; i < mp->read_count; i++)
	{
		pv[i].value_raw = inputs_raw[i];
		pv[i].value_scaled = (float) inputs_raw[i] * pv[i].scale;
	}

	printf("\r");
	for (i = 0; i < mp->read_count; i++)
		printf("%16.2f", pv[i].value_scaled);
	fflush(stdout);	
}

int posmatch (int maj, int min, int read_count)
{
	int i;
	for (i = 0; i < read_count; i++)
	{
		if (pv[i].major == maj &&
			pv[i].minor == min)
		{
			return i;
		}
	}
	return -1;
}

void write_registers_tofile(modbusport *mp)
{
	FILE *fp;
	int i, j;
	char *logfilename = gen_filename(uuid);
	int pathlength = strlen(logfilename) + strlen(SENSORDATA);
	char logpath[pathlength + 1];

	strcpy(logpath, SENSORDATA);
	strcat(logpath, logfilename);
	if ((fp = fopen(logpath, "w")) == NULL)
	{
		fail("Error opening sensor log file for writing register reads", NULL);
	}

	for (j = 0; j < mp->read_count; j++)
	{
		int ix = posmatch(j, 0, mp->read_count);
		if (ix == -1)
			continue;
		char outstring[512];
		char buf[80];
		strcpy(outstring, "<D>,SEC:PUBLIC");

		for (i = 0; i < mp->read_count; i++)
		{
			ix = posmatch(j, i, mp->read_count);
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
}


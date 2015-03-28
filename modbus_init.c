#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "modbus_init.h"
#include "parse.h"
#include "read.h"
#include "time.h"
#include "shared.h"

#define UUID_FILE "/uuid"
#define INTERVAL_MIN 1
#define INTERVAL_MAX 3600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'

int paramcount;
double delaytime;
FILE *fp = NULL;

static int line = 1;
static int column = 1;

void fatal (char * errstr, modbusport *mp)
{
	err(errstr, mp);
	if (mp != NULL)
	{
		modbus_close(mp->port);
		modbus_free(mp->port);
	}
	logger("exiting.", mp);
	exit(-1);
}

void err (char *errstr, modbusport *mp)
{
	char ts[TIMESTAMP_LEN];
	timestamp(ts);
	fprintf(stderr, "[%s][%s]:ERROR: %s %s\n",
			ts, mp->dname, errstr, strerror(errno));
}

void logger (char *str, modbusport *mp)
{
	char ts[TIMESTAMP_LEN];
	timestamp(ts);
	fprintf(stderr, "[%s][%s]:LOG: %s\n",
			ts, mp->dname, str);
}

void get_modbus_params(modbusport *mp)
{
        if (access(CONF_FILE, F_OK) != 0)
        {
		logger("\"" CONF_FILE "\" no such file. Using defaults.", mp);
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

        if (mp->secs < INTERVAL_MIN || mp->secs > INTERVAL_MAX)
        {
                fprintf(stderr, "Error in configuration file '%s' : parameter \n"
                        "'interval_secs' must be between %d and %d.\n"
                        "You have entered a value of %ld\n",
                        CONF_FILE, INTERVAL_MIN, INTERVAL_MAX, mp->secs);
                exit(-1);
        }

}

modbus_t *modbus_init (modbusport *mp, element *pv)
{

	int i;
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
		fatal("Failed to set modbus slave address", mp);

	if (modbus_connect(mp->port))
		fatal("Unable to connect to modbus server", mp);
#endif /* NOMODBUS */

	printf("\n");
	for (i = 0; i < mp->read_count; i++)
	{
		printf("%16s", pv[i].id);
	}
	printf("\n");
	return mp->port;
}

void ile_aip_init(modbusport *mp)
{
	FILE *fp;
	if ((fp = fopen(UUID_FILE, "r")) == NULL)
		fatal("Failed to open UUID file", NULL);
	fgets(mp->uuid, UUID_LENGTH, fp);
	if (strlen(mp->uuid) != UUID_LENGTH - 1)
	{
		fprintf(stderr, 
			"Error : file '%s' does not contain a UUID in the expected format\n%s\n%d\n",
			UUID_FILE, mp->uuid, strlen(mp->uuid));
		exit(-1);
	}
	if (access(SENSORDATA, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", SENSORDATA, strerror(errno));
		exit(-1);
	}
}

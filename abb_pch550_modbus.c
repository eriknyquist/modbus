#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "abb_pch550_modbus.h"
#include "abb_pch550_time.h"

#define SENSORDATA "/home/sensordata/"
#define UUID_LENGTH 37
#define UUID_FILE "/uuid"
#define MB_BITRATE 9600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 10

double delaytime;
char uuid[38];

element sv[REG_READ_COUNT];
element *pv[REG_READ_COUNT];

char *descriptors[REG_READ_COUNT] =
{
	"SENS_OUTPUT_FREQ_HZ",
	"SENS_CURRENT_A",
	"SENS_VOLTAGE_V",
	"SENS_MOTORSPEED_RPM",
	"SENS_KW",
	"SENS_KWH"
};

float scalefactors[REG_READ_COUNT] =
{
        FREQ_RESOLUTION_HZ,
        CURRENT_RESOLUTION_A,
        VOLTAGE_RESOLUTION_V,
        MOTORSPEED_RESOLUTION_RPM,
	KW_RESOLUTION,
	KWH_RESOLUTION
};

void fail (char * errstr, modbus_t *modbusport)
{
	fprintf(stderr, "\n%s\n%s\n", errstr, strerror(errno));
	if (modbusport != NULL)
	{
		modbus_close(modbusport);
		modbus_free(modbusport);
	}
	exit(-1);
}

modbus_t *abb_pch550_modbus_init (char *serialport)
{
	modbus_t *modbusport;

	if (access(serialport, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", serialport, strerror(errno));
		exit(-1);
	}

	modbusport = modbus_new_rtu(serialport, MB_BITRATE, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (modbusport == NULL)
	{
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n%s\n",
			serialport, 
			strerror(errno));
		exit(-1);
	}

	if (modbus_set_slave(modbusport, MB_SLAVE_ADDRESS))
		fail("Failed to set modbus slave address", modbusport);

	if (modbus_connect(modbusport))
		fail("Unable to connect to modbus server", modbusport);
	int i;
	for (i = 0; i < REG_READ_COUNT; i++)
	{
		pv[i] = &sv[i];
		pv[i]->desc = descriptors[i];
	}
	
	return modbusport;
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

int abb_pch550_read (uint16_t *inputs_raw, modbus_t *modbusport)
{
	int n, i;

	n = modbus_read_registers(modbusport, REG_READ_BASE, REG_READ_COUNT, inputs_raw);

	if (n <= 0)
	{
		fail("Unable to read modbus registers", modbusport);
	}

	for (i = 0; i < REG_READ_COUNT; i++)
	{
		pv[i]->value_raw = inputs_raw[i];
		pv[i]->value_scaled = (float) inputs_raw[i] * scalefactors[i];
	}

	/* ---debug--- */
	printf("\r%16.2f%16.2f%16.2f%16.2f%16.2f%16.2f", pv[0]->value_scaled, pv[1]->value_scaled,
		pv[2]->value_scaled, pv[3]->value_scaled, pv[4]->value_scaled,
		pv[5]->value_scaled);
	fflush(stdout);
	/* ----------- */
}

void write_registers_tofile(modbus_t *modbusport)
{
	FILE *fp;
	int i;
        char outstring[512];
	char *logfilename = gen_filename(uuid);
	int pathlength = strlen(logfilename) + strlen(SENSORDATA);
	char logpath[pathlength + 1];

	strcpy(logpath, SENSORDATA);
	strcat(logpath, logfilename);

        strcpy(outstring, "<D>,SEC:PUBLIC,");

	if ((fp = fopen(logpath, "w")) == NULL)
	{
		fail("Error opening sensor log file for writing register reads", modbusport);
	}

	for (i = 0; i < REG_READ_COUNT; i++)
	{
		char buf[80];
		snprintf(buf, sizeof(buf),
			(i < (REG_READ_COUNT - 1)) ? "%s:%.2f," : "%s:%.2f",
			pv[i]->desc, pv[i]->value_scaled);
		strcat(outstring, buf);
	}
	fputs(outstring, fp);
	fclose(fp);
}



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
#include "abb_pch550_file.h"

#define MB_BITRATE 9600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 10

double delaytime;

element sv[REG_READ_COUNT];
element *pv[REG_READ_COUNT];

char *descriptors[REG_READ_COUNT] =
{
	"SENS_OUTPUT_FREQ_HZ",
	"SENS_CURRENT_A",
	"SENS_VOLTAGE_V",
	"SENS_MOTORSPEED_RPM"
};

float scalefactors[REG_READ_COUNT] =
{
        FREQ_RESOLUTION_HZ,
        CURRENT_RESOLUTION_A,
        VOLTAGE_RESOLUTION_V,
        MOTORSPEED_RESOLUTION_RPM
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
	{
		fail("Failed to set modbus slave address", modbusport);
	}  

	if (modbus_connect(modbusport))
	{
		fail("Unable to connect to modbus server", modbusport);
	}
	int i;
	for (i = 0; i < REG_READ_COUNT; i++)
	{
		pv[i] = &sv[i];
		pv[i]->desc = descriptors[i];
	}
	
	return modbusport;
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
	printf("\r%16.2f%16.2f%16.2f%16.2f", pv[0]->value_scaled, pv[1]->value_scaled,
		pv[2]->value_scaled, pv[3]->value_scaled);
	fflush(stdout);
	/* ----------- */
}

void write_registers_tofile(char *filename, modbus_t *modbusport)
{
	FILE *fp;
	int i;
	char *timestamp_string = timestamp();

	if ((fp = fopen(filename, "a")) == NULL)
	{
		fail("Error opening sensor log file for writing register reads", modbusport);
	}

	for (i = 0; i < REG_READ_COUNT; i++)
	{
		if (write_register_tofile(fp, timestamp_string, pv[i]) != 0)
		{
			fail("Error writing register read to sensor log file", modbusport);
		}	
	}
	fputs("\n", fp);
	fclose(fp);
}

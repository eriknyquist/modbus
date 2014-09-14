#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "abb_time.h"

#define MB_BITRATE 9600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 10

double delaytime;

typedef struct element
{
	uint16_t value_raw;
	float value_scaled;
	char *desc;
} element;

element sv[REG_READ_COUNT];
element *pv[REG_READ_COUNT];

char *descriptors[REG_READ_COUNT] =
{
	"OUTPUT_FREQ_HZ",
	"CURRENT_A",
	"VOLTAGE_V",
	"MOTORSPEED_RPM"
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

modbus_t *abb_modbus_init (char *serialport)
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

int abb_update_input_registers (uint16_t *inputs_raw, modbus_t *modbusport)
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
}

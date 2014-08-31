#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <modbus.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "common.h"

#define MB_BITRATE 9600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 10

float scalefactors[4] =
{
        FREQ_RESOLUTION_HZ,
        CURRENT_RESOLUTION_A,
        VOLTAGE_RESOLUTION_V,
        MOTORSPEED_RESOLUTION_RPM
};

void fail (modbus_t *mbp)
{
	modbus_close(mbp);
	modbus_free(mbp);
	exit(-1);
}

double getms ()
{
	struct timeval time;
	gettimeofday (&time, NULL);
	return (time.tv_sec + (time.tv_usec / 1000000.0)) * 1000.0;
}

void abb_modbus_init (char *serialport, modbus_t *modbusport)
{
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
		fprintf(stderr, "Failed to set modbus slave address\n%s\n", strerror(errno));
		fail(modbusport);
	}  

	if (modbus_connect(modbusport))
	{
		fprintf(stderr, "Unable to connect to modbus server\n%s\n", strerror(errno));
		fail(modbusport);
	}
}

void update (uint16_t *inputs_raw, float *inputs_scaled, modbus_t *mbp)
{
        int n, i;
        n = modbus_read_input_registers(mbp, READ_BASE, READ_COUNT, inputs_raw);

        if (n <= 0)
        {
                fprintf(stderr, "Unable to read modbus registers\n%s\n", strerror(errno));
                fail(mbp);
        }

        for (i = 0; i < READ_COUNT; i++)
        {
                inputs_scaled[i] = inputs_raw[i] * scalefactors[i];
        }
}

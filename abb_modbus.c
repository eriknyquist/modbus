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

double delaytime;
static double lastupdate;

float scalefactors[4] =
{
        FREQ_RESOLUTION_HZ,
        CURRENT_RESOLUTION_A,
        VOLTAGE_RESOLUTION_V,
        MOTORSPEED_RESOLUTION_RPM
};

void fail (char * errstr, modbus_t *modbusport)
{
	fprintf(stderr, "\n%s\n%s\n", errstr, strerror(errno));
	modbus_close(modbusport);
	modbus_free(modbusport);
	exit(-1);
}

double getms ()
{
	struct timeval time;
	gettimeofday (&time, NULL);
	return (time.tv_sec + (time.tv_usec / 1000000.0)) * 1000.0;
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

	delaytime = 1000 / UPDATE_FREQUENCY_HZ;
	lastupdate = (getms() - delaytime);
	return modbusport;
}

void abb_read_input_registers (uint16_t *inputs_raw, float *inputs_scaled, modbus_t *modbusport)
{
        int n, i;
        n = modbus_read_input_registers(modbusport, INPUT_REG_READ_BASE, INPUT_REG_READ_COUNT, inputs_raw);

        if (n <= 0)
        {
                fail("Unable to read modbus registers", modbusport);
        }

        for (i = 0; i < INPUT_REG_READ_COUNT; i++)
        {
                inputs_scaled[i] = inputs_raw[i] * scalefactors[i];
        }
}

int abb_update_input_registers (uint16_t *inputs_raw, float *inputs_scaled, modbus_t *modbusport)
{
	int i;
	if ((getms() - lastupdate) >= delaytime)
	{
		abb_read_input_registers(inputs_raw, inputs_scaled, modbusport);

		/* ---debug--- */
		printf("\r");
		for (i = 0; i < INPUT_REG_READ_COUNT; i++)
		{
			printf ( "%16f", (inputs_scaled[i]));
		}
		fflush(stdout);
		/* ----------- */

		lastupdate = getms();
		return 1;
	}
	return 0;
}

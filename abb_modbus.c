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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "modbus.h"
#include <errno.h>

#define MB_BITRATE 19200
#define MB_DATABITS 8
#define MB_STOPBITS 1
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 1

#define MB_REG_BASE 6
#define MB_REG_COUNT 4

void fail (modbus_t *ctx)
{
	modbus_close(ctx);
	modbus_free(ctx);
	exit(-1);
}

int main ( int argc, char *argv[] )
{
	modbus_t *ctx;
  	uint16_t tab_reg[MB_REG_BASE + MB_REG_COUNT];
	int n, i;

	if (argc != 2)
	{
		fprintf(stderr, "Syntax: %s serial_device\n", argv[0]);
		return -1;
	}

	ctx = modbus_new_rtu(argv[1], MB_BITRATE, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (ctx == NULL)
	{
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n", argv[1]);
		return -1;
	}

	if (modbus_set_slave(ctx, MB_SLAVE_ADDRESS))
	{
		fprintf(stderr, "Failed to set modbus slave address\n");
		fail(ctx);
	}  

	if (modbus_connect(ctx))
	{
		fprintf(stderr, "Unable to connect to modbus server");
		fail(ctx);
	}

	n = modbus_read_registers(ctx, MB_REG_BASE, MB_REG_COUNT, tab_reg+MB_REG_BASE);

	if (n <= 0)
	{
		fprintf(stderr, "Unable to read modbus registers\n");
		fail(ctx);
	}

	for (i = MB_REG_BASE; i < MB_REG_BASE + n; i++)
	{
		printf ( "[%d]: %d\n", i, tab_reg[i]);
	}
}

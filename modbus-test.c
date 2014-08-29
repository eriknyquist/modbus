#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <modbus.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define MB_BITRATE 9600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 10
#define UPDATE_FREQUENCY_HZ 10

#define ADDRESS_START 0
#define ADDRESS_END 99

#define READ_BASE 5
#define READ_COUNT 4

uint8_t gotsigint = 0;

void fail (modbus_t *ctx)
{
	modbus_close(ctx);
	modbus_free(ctx);
	exit(-1);
}

double getms ()
{
	struct timeval time;
	gettimeofday (&time, NULL);
	return (time.tv_sec + (time.tv_usec / 1000000.0)) * 1000.0;
}

void siginthandler()
{
	gotsigint = 1;
}

int main ( int argc, char *argv[] )
{
	double delaytime = 1000 / UPDATE_FREQUENCY_HZ;
	double lastupdate;
  	int nb, n, i;
	uint16_t *tab_registers;
	modbus_t *ctx;

	nb = ADDRESS_END - ADDRESS_START;
	tab_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
	memset(tab_registers, 0, nb * sizeof(uint16_t));

	signal(SIGINT, siginthandler);

	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./%s <serial_device>\n", argv[0]);
		return -1;
	}

	ctx = modbus_new_rtu(argv[1], MB_BITRATE, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (ctx == NULL)
	{
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n%s\n", argv[1], strerror(errno));
		return -1;
	}

	if (modbus_set_slave(ctx, MB_SLAVE_ADDRESS))
	{
		fprintf(stderr, "Failed to set modbus slave address\n%s\n", strerror(errno));
		fail(ctx);
	}  

	if (modbus_connect(ctx))
	{
		fprintf(stderr, "Unable to connect to modbus serveri\n%s\n", strerror(errno));
		fail(ctx);
	}

	printf("Freq.     Current   Voltage   Motor RPM \n");

	lastupdate = (getms() - delaytime);
	while(1)
	{
		if ((getms() - lastupdate) >= delaytime)
		{
			n = modbus_read_registers(ctx, READ_BASE, READ_COUNT, tab_registers);

			if (n <= 0)
			{
				fprintf(stderr, "Unable to read modbus registers\n%s\n", strerror(errno));
				fail(ctx);
			}

			printf("\r");

			for (i = READ_BASE; i < READ_COUNT; i++)
			{
				printf ( "%10d", tab_registers[i]);
			}

			fflush(stdout);
			lastupdate = getms();
		}

		if (gotsigint) fail(ctx); 
	}
}

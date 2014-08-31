#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <modbus.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "abb_modbus.h"
#include "common.h"

static uint8_t gotsigint = 0;
static modbus_t *modbusport;

void siginthandler()
{
	gotsigint = 1;
}

struct values
{
	uint16_t *inputs_raw;
	float *inputs_scaled;
};

int main ( int argc, char *argv[] )
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./%s <serial_device>\n", argv[0]);
		return -1;
	}

	double delaytime = 1000 / UPDATE_FREQUENCY_HZ;
	double lastupdate;
  	int n, i;
	struct values *registers, r;
	registers = &r;

	/* Allocate space to store register reads */
	registers->inputs_raw = (uint16_t *) malloc(READ_COUNT * sizeof(uint16_t));
	memset(registers->inputs_raw, 0, READ_COUNT * sizeof(uint16_t));

	registers->inputs_scaled = (float *) malloc(READ_COUNT * sizeof(float));
	memset(registers->inputs_scaled, 0, READ_COUNT * sizeof(float));

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	abb_modbus_init(argv[0], modbusport);	

	printf("         Freq.(Hz)       Current(A)      Voltage(V)      Motor Speed(RPM) \n");

	/* initialise like this so we get a print immediately */
	lastupdate = (getms() - delaytime);
	while(1)
	{
		if ((getms() - lastupdate) >= delaytime)
		{
			update(registers->inputs_raw, registers->inputs_scaled, modbusport);
			printf("\r");
			for (i = 0; i < READ_COUNT; i++)
			{
				printf ( "%16f", (registers->inputs_scaled[i]));
			}

			fflush(stdout);
			lastupdate = getms();
		}
		
		/* if we caught sigint, close modbus
		   connections & exit gracefully */
		if (gotsigint) fail(modbusport); 
	}
}

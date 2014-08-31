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

uint8_t gotsigint = 0;
static modbus_t *mbp;

float scalefactors[4] =
{
	FREQ_RESOLUTION_HZ, 
	CURRENT_RESOLUTION_A,
	VOLTAGE_RESOLUTION_V, 
	MOTORSPEED_RESOLUTION_RPM
};

void siginthandler()
{
	gotsigint = 1;
}

struct values
{
	uint16_t *inputs_raw;
	float *inputs_scaled;
};

void update (uint16_t *inputs_raw, float *inputs_scaled)
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
	struct values *v;

	/* Allocate space to store register reads */
	v->inputs_raw = (uint16_t *) malloc(READ_COUNT * sizeof(uint16_t));
	memset(v->inputs_raw, 0, READ_COUNT * sizeof(uint16_t));

	v->inputs_scaled = (float *) malloc(READ_COUNT * sizeof(float));
	memset(v->inputs_scaled, 0, READ_COUNT * sizeof(float));

	/* Catch sigint (Ctrl-C) */
	signal(SIGINT, siginthandler);

	abb_modbus_init(argv[0], mbp);	

	printf("         Freq.(Hz)       Current(A)      Voltage(V)      Motor Speed(RPM) \n");

	/* initialise like this so we get a print immediately */
	lastupdate = (getms() - delaytime);
	while(1)
	{
		if ((getms() - lastupdate) >= delaytime)
		{
			update(v->inputs_raw, v->inputs_scaled);
			printf("\r");
			for (i = 0; i < READ_COUNT; i++)
			{
				printf ( "%16f", (v->inputs_scaled[i]));
			}

			fflush(stdout);
			lastupdate = getms();
		}
		
		/* if we caught sigint, close modbus
		   connections & exit gracefully */
		if (gotsigint) fail(mbp); 
	}
}

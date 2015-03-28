#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "modbus_init.h"
#include "time.h"
#include "shared.h"

int abb_ach550_read (uint16_t *inputs_raw, modbusport *mp, element *pv)
{
	int n, i;

#ifndef NOMODBUS
	n = modbus_read_registers(mp->port, mp->read_base, mp->read_count, inputs_raw);
	if (n <= 0)
	{
		fail("Unable to read modbus registers", mp->port);
	}
#endif
	for (i = 0; i < mp->read_count; i++)
	{
		pv[i].value_raw = inputs_raw[i];
		pv[i].value_scaled = (float) inputs_raw[i] * pv[i].scale;
	}

	printf("\r");
	for (i = 0; i < mp->read_count; i++)
		printf("%16.2f", pv[i].value_scaled);
	fflush(stdout);	
}

int posmatch (int maj, int min, int read_count, element *pv)
{
	int i;
	for (i = 0; i < read_count; i++)
	{
		if (pv[i].major == maj &&
			pv[i].minor == min)
		{
			return i;
		}
	}
	return -1;
}

void write_registers_tofile(modbusport *mp, element *pv)
{
	FILE *fp;
	int i, j;
	char *logfilename = gen_filename(mp->uuid);
	int pathlength = strlen(logfilename) + strlen(SENSORDATA);
	char logpath[pathlength + 1];

	strcpy(logpath, SENSORDATA);
	strcat(logpath, logfilename);
	if ((fp = fopen(logpath, "w")) == NULL)
	{
		fail("Error opening sensor log file for writing register reads", NULL);
	}

	for (j = 0; j < mp->read_count; j++)
	{
		int ix = posmatch(j, 0, mp->read_count, pv);
		if (ix == -1)
			continue;
		char outstring[512];
		char buf[80];
		strcpy(outstring, "<D>,SEC:PUBLIC");

		for (i = 0; i < mp->read_count; i++)
		{
			ix = posmatch(j, i, mp->read_count, pv);
			if (ix == -1)
				continue;

			snprintf(buf, sizeof(buf), ",%s:%.2f",
				pv[ix].tag, pv[ix].value_scaled);
			strcat(outstring, buf);
		}
		fputs(outstring, fp);
		fputc('\n', fp);
	}
	fclose(fp);
}
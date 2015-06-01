#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "init.h"
#include "time.h"
#include "log.h"

#define SENSOR_READING_HEADER "<D>,SEC:PUBLIC"

/* TODO: put this ptr inside modbusport struct */
extern uint16_t *inputs_raw;

void mbd_read (modbusport *mp, element *pv)
{
	int i;

#ifndef NOMODBUS
	int n;
	char msg[MAX_LOG_LEN];
	snprintf(msg, sizeof(msg), "reading modbus registers %d to %d from '%s'",
		mp->read_base, mp->read_base + mp->read_count, mp->port_name);
	logger(msg, mp);

	n = modbus_read_registers(mp->port, mp->read_base, mp->read_count, inputs_raw);
	if (n <= 0)
	{
		fatal("Unable to read modbus registers", mp);
	}
#endif
	for (i = 0; i < mp->read_count; i++)
	{
		pv[i].value_raw = inputs_raw[i];
		pv[i].value_scaled = (float) inputs_raw[i] * pv[i].scale;
	}

#ifdef DEBUG
	printf("\r");
	for (i = 0; i < mp->read_count; i++)
		printf("%16.2f", pv[i].value_scaled);
	fflush(stdout);	
#endif
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
	char logpath[MAX_PATH_LEN];
	snprintf(logpath, sizeof(logpath), "%s/%s", SENSORDATA, logfilename);

	logger("writing to " SENSORDATA, mp);
	if ((fp = fopen(logpath, "w")) == NULL)
	{
		err("can't open " SENSORDATA " to write data", mp);
	}

	for (j = 0; j < mp->read_count; j++)
	{
		int ix = posmatch(j, 0, mp->read_count, pv);
		if (ix == -1)
			continue;
		char outstring[512];
		char buf[80];
		strcpy(outstring, SENSOR_READING_HEADER);

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
	free(logfilename);
}

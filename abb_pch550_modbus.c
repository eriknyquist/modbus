#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "abb_pch550_modbus.h"
#include "abb_pch550_time.h"

#define CONF_FILE "abb.conf"
#define SENSORDATA "/home/sensordata/"
#define UUID_LENGTH 37
#define UUID_FILE "/uuid"
#define MB_BITRATE 9600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 10

int modbus_rtu_baud, modbus_station_id,
	modbus_read_base, modbus_read_count,
	update_frequency_hz;

double delaytime;
char uuid[38];

element *pv;

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

uint8_t is_id (char c)
{
	return (c <= 'z'&& c >= 'a' || c <= 'Z' && c >= 'A' ||
		c == '_') ? 1 : 0;
}

uint8_t is_dec (char c)
{
	return (c <= '9' && c >= '0' || c == '.') ? 1 : 0 ;
}

uint8_t is_whitespace (char c)
{
	return (c == ' ' || c == '\t' || c == '\r' ||
		c == '\v' || c == '\n') ? 1 : 0;
}

void syntaxerr (int line, char c)
{
	fprintf(stderr, "Syntax error '%c' in configuration file %s, line %d\n",
		c, CONF_FILE, line);
	exit(-1);
}

void assign (char *param, char *value)
{
	if (strcmp(param, "modbus_rtu_baud") == 0)
		modbus_rtu_baud = atoi(value);
	else if (strcmp(param, "modbus_station_id") == 0)
		modbus_station_id = atoi(value);
	else if (strcmp(param, "modbus_read_base") == 0)
		modbus_read_base = atoi(value);
	else if (strcmp(param, "modbus_read_count") == 0)
		modbus_read_count = atoi(value);
	else if (strcmp(param, "update_frequency_hz") == 0)
		update_frequency_hz = atof(value);
	else
	{
		fprintf(stderr,
			"Error parsing %s:\nunrecognised parameter '%s'\n",
			CONF_FILE, param);
		exit(-1);
	}
	printf("value %s assigned to parameter '%s'\n", value, param);
}

element get_element (FILE *fp, int line)
{
	element e;
	int i;
	int state = 0;
	char idbuf[80], pbuf[80], tag[80], scale[10], c;	
	int idbufpos = 0, pbufpos = 0, tagpos = 0, scalepos = 0;

	while (state != 5)
	{
		c = fgetc(fp);
		switch (state)
		{
			case 0:
				if (is_id(c))
				{
					idbuf[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (is_whitespace(c))
					state = 0;
				else
					syntaxerr(line, c);
			break;
			case 1:
				if (is_id(c))
				{
					idbuf[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (is_whitespace(c))
					state = 1;
				else if (c == '{')
				{
					idbuf[idbufpos] = '\0';
					state = 2;
				}
				else
					syntaxerr(line, c);
			break;
			case 2:
				if (is_id(c))
				{
					pbuf[pbufpos] = c;
					pbufpos++;
					state = 2;
				}
				else if (is_whitespace(c))
					state = 2;
				else if (c == '=')
				{
					pbuf[pbufpos] = '\0';
					if (strcmp(pbuf, "name") == 0)
						state = 3;
					else if (strcmp(pbuf, "scale") == 0)
						state = 4;
					else
						syntaxerr(line, c);
					pbufpos = 0;
				}
				else
					syntaxerr(line, c);
			break;
			case 3:
				if (is_id(c))
				{
					tag[tagpos] = c;
					tagpos++;
					state = 3;
				}
				else if (is_whitespace(c))
					state = 3;
				else if (c == ',')
				{
					tag[tagpos] = '\0';
					state = 2;
				}
				else if (c == '}')
				{
					tag[tagpos] = '\0';
					state = 5;
				}
				else
					syntaxerr(line, c);
			break;
			case 4:
				if (is_dec(c))
				{
					scale[scalepos] = c;
					scalepos++;
					state = 4;
				}
				else if (is_whitespace(c))
					state = 4;
				else if (c == ',')
				{
					scale[scalepos] = '\0';
					state = 2; 
				}
				else if (c == '}')
				{
					scale[scalepos] = '\0';
					state = 5;
				}
				
			break;
		}
		if (c == '\n') line++;
	}
	e.tag = tag;
	e.id = idbuf;
	e.scale = atof(scale);
	return e;
}

void parse_conf(FILE *fp, int line)
{
	char c;
	uint8_t isfloat = 0, state = 0, idbufpos = 0, valbufpos = 0;
	char idbuf[80], valbuf[32];

	c = fgetc(fp);
	while (c != ';')
	{
		switch (state)
		{
			case 0:
				if (is_id(c))
				{
					idbuf[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (is_whitespace(c))
					state = 0;
				else if (c == '#')
					state = 3;
				else
					syntaxerr(line, c);
			break;
			case 1:
				if (is_id(c))
				{
					idbuf[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (c == '=')
				{
					idbuf[idbufpos] = '\0';
					state = 2;
				}
				else
					syntaxerr(line, c);
			break;
			case 2:
				if (is_dec(c))
				{
					valbuf[valbufpos] = c;
					valbufpos++;
					state = 2;
					if (c == '.') isfloat = 1;
				}
				else if (c == ',')
				{
					valbuf[valbufpos] = '\0';
					assign(idbuf, valbuf);
					state = 0;
					idbufpos = 0;
					valbufpos = 0;
				}
				else if (is_whitespace(c))
					state = 2;
				else
					syntaxerr(line, c);
			break;
			case 3:
				if (c == '\n')
					state = 0;
				else
					state = 3;
			break;
		}
		if (c == '\n') line++;
		c = fgetc(fp);
	}
	valbuf[valbufpos] = '\0';
	assign(idbuf, valbuf);
	printf("Done parsing!\n");
}

modbus_t *abb_pch550_modbus_init (char *serialport)
{
	FILE *fp;
	if ((fp = fopen(CONF_FILE, "r")) == NULL)
	{
		fprintf(stderr, "Error opening file '%s' for reading:\n%s\n",
			CONF_FILE, strerror(errno));
		exit(-1);
	}
	int i;
	static int line = 1;
	parse_conf(fp, line);
	pv = malloc(sizeof(element) * modbus_read_count);

	for (i = 0; i < modbus_read_count; i++)
	{
		pv[i] = get_element(fp, line);
		printf("tag=%s, id=%s, scale=%.1f\n", pv[i].tag, pv[i].id, pv[i].scale);
		
	}
	
	for (i = 0; i < modbus_read_count; i++)
	{
		printf("tag=%s, id=%s, scale=%.1f\n", pv[i].tag, pv[i].id, pv[i].scale);
	}

	modbus_t *modbusport;
/*
	if (access(serialport, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", serialport, strerror(errno));
		exit(-1);
	}

	modbusport = modbus_new_rtu(serialport, modbus_rtu_baud, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (modbusport == NULL)
	{
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n%s\n",
			serialport, 
			strerror(errno));
		exit(-1);
	}

	if (modbus_set_slave(modbusport, MB_SLAVE_ADDRESS))
		fail("Failed to set modbus slave address", modbusport);

	if (modbus_connect(modbusport))
		fail("Unable to connect to modbus server", modbusport);
*/	
	return NULL;
}

void ile_aip_init(void)
{
	FILE *fp;
	if ((fp = fopen(UUID_FILE, "r")) == NULL)
		fail("Failed to open UUID file", NULL);
	fgets(uuid, UUID_LENGTH, fp);
	if (strlen(uuid) != UUID_LENGTH - 1)
	{
		fprintf(stderr, 
			"Error : file '%s' does not contain a UUID in the expected format\n%s\n%d\n",
			UUID_FILE, uuid, strlen(uuid));
		exit(-1);
	}
	if (access(SENSORDATA, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", SENSORDATA, strerror(errno));
		exit(-1);
	}
}

int abb_pch550_read (uint16_t *inputs_raw, modbus_t *modbusport)
{
	int n, i;

	/*n = modbus_read_registers(modbusport, REG_READ_BASE, REG_READ_COUNT, inputs_raw);

	if (n <= 0)
	{
		fail("Unable to read modbus registers", modbusport);
	}*/

	for (i = 0; i < modbus_read_count; i++)
	{
		inputs_raw[i] = i * 3;
		pv[i].value_raw = inputs_raw[i];
		pv[i].value_scaled = (float) inputs_raw[i] * pv[i].scale;
	}

	/* ---debug--- */
	printf("\r%16.2f%16.2f%16.2f%16.2f%16.2f%16.2f", pv[0].value_scaled, pv[1].value_scaled,
		pv[2].value_scaled, pv[3].value_scaled, pv[4].value_scaled,
		pv[5].value_scaled);
	fflush(stdout);
	/* ----------- */
}

void write_registers_tofile(modbus_t *modbusport)
{
	FILE *fp;
	int i;
        char outstring[512];
	char *logfilename = gen_filename(uuid);
	int pathlength = strlen(logfilename) + strlen(SENSORDATA);
	char logpath[pathlength + 1];

	strcpy(logpath, SENSORDATA);
	strcat(logpath, logfilename);

        strcpy(outstring, "<D>,SEC:PUBLIC,");

	if ((fp = fopen(logpath, "w")) == NULL)
	{
		fail("Error opening sensor log file for writing register reads", modbusport);
	}

	for (i = 0; i < modbus_read_count; i++)
	{
		char buf[80];
		snprintf(buf, sizeof(buf),
			(i < (modbus_read_count - 1)) ? "%s:%.2f," : "%s:%.2f",
			pv[i].tag, pv[i].value_scaled);
		strcat(outstring, buf);
	}
	fputs(outstring, fp);
	fclose(fp);
}


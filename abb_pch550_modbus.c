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

#define CONF_FILE "/etc/abb.conf"
#define SENSORDATA "/home/sensordata/"
#define PARAMS 6
#define UUID_LENGTH 37
#define UUID_FILE "/uuid"
#define MB_BITRATE 9600
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'
#define MB_SLAVE_ADDRESS 10

int modbus_rtu_baud = 9600, modbus_station_id = 0,
	modbus_read_base = 0, modbus_read_count = 1;
float update_frequency_hz = 0.5;
char modbus_port_name[128] = "/dev/null";

int paramcount;
double delaytime;
char uuid[38];

element *pv;
static int line = 1;

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

void syntaxerr (char c)
{
	fprintf(stderr, "Syntax error '%c' in configuration file %s, line %d\n",
		c, CONF_FILE, line);
	exit(-1);
}

void unrec_id(char *id)
{
	fprintf(stderr,
		"Unrecognised identifier '%s' in configuration file %s, line %d\n",
		id, CONF_FILE, line);
	exit(-1);
}

void nosuchparam(char *param)
{
	fprintf(stderr, "Error in configuration file %s, line %d\n", CONF_FILE, line);
	fprintf(stderr, "'%s' : no such parameter.\n", param);
	exit(-1);
}

void doubleassn (char *id)
{
	fprintf(stderr, "Error in configuration file %s, line %d\n", CONF_FILE, line);
	fprintf(stderr, "ID '%s' has already been assigned a position.\n", id);
	fprintf(stderr, "You cannot assign two positions to the same ID.\n");
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
	else if (strcmp(param, "modbus_port") == 0);
	else
		nosuchparam(param);

	printf("%24s : %s\n", param, value);
	paramcount++;
}

element get_element (FILE *fp)
{
	element e;
	int i;
	int state = 0;
	char pbuf[80], scale[10], c;	
	int idbufpos = 0, pbufpos = 0, tagpos = 0, scalepos = 0;

	while (state != 6)
	{
		c = fgetc(fp);
		if (c == '\n')
			line++;
		switch (state)
		{
			case 0:
				if (is_id(c))
				{
					e.id[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (is_whitespace(c))
					state = 0;
				else if (c == '#')
					state = 5;
				else
					syntaxerr(c);
			break;
			case 1:
				if (is_id(c))
				{
					e.id[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (is_whitespace(c))
					state = 1;
				else if (c == '{')
				{
					e.id[idbufpos] = '\0';
					state = 2;
				}
				else
					syntaxerr(c);
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
					if (strcmp(pbuf, "tag") == 0)
						state = 3;
					else if (strcmp(pbuf, "scale") == 0)
						state = 4;
					else
						nosuchparam(pbuf);
					pbufpos = 0;
				}
				else
					syntaxerr(c);
			break;
			case 3:
				if (is_id(c))
				{
					e.tag[tagpos] = c;
					tagpos++;
					state = 3;
				}
				else if (is_whitespace(c))
					state = 3;
				else if (c == ',')
				{
					e.tag[tagpos] = '\0';
					state = 2;
				}
				else if (c == '}')
				{
					e.tag[tagpos] = '\0';
					state = 6;
				}
				else
					syntaxerr(c);
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
					state = 6;
				}
				
			break;
			case 5:
				if (c == '\n')
					state = 0;
			break;
		}
	}
	e.scale = atof(scale);
	return e;
}

int idcmp (char *idc)
{
	int i;
	for (i = 0; i < modbus_read_count; i++)
	{
		if (strcmp(idc, pv[i].id) == 0)
			return i;
	}
	return -1;
}

void kvp_conf (FILE *fp)
{
	int majc = 0, minc = 0;
	char idbuf[80], c;
	int idbufpos = 0;
	uint8_t state = 0;
	while ((c = fgetc(fp)) != EOF)
	{
		if (c == '\n')
			line++;
		switch (state)
		{
			case 0:
				if (c == '{')
					state = 1;
				else if (is_whitespace(c))
					state = 0;
				else if (c == '#')
					state = 2;
				else
					syntaxerr(c);
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
				else if (c == ',' || c == '}')
				{
					idbuf[idbufpos] = '\0';
					int ti = idcmp(idbuf);
					if (ti == -1)
						unrec_id(idbuf);
					if (pv[ti].major != -1)
						doubleassn(idbuf);
					pv[ti].major = majc;
					pv[ti].minor = minc;
					idbufpos = 0;
					if (c == '}')
					{
						majc++;
						minc = 0;
						state = 0;
					}
					else
						minc++;
				}
				else
					syntaxerr(c);
			break;
			case 2:
				if (c == '\n')
					state = 0;
			break;
		}
	}
}

void parse_conf(FILE *fp)
{
	char c;
	uint8_t isfloat = 0, state = 0, idbufpos = 0, valbufpos = 0;
	char idbuf[80], valbuf[32];

	c = fgetc(fp);
	while (c != ';')
	{
		if (c == '\n')
			line++;
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
					syntaxerr(c);
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
					state = (strcmp(idbuf, "modbus_port") == 0)
						? 4 : 2;
				}
				else
					syntaxerr(c);
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
					syntaxerr(c);
			break;
			case 3:
				if (c == '\n')
					state = 0;
				else
					state = 3;
			break;
			case 4:
				if (c == ',')
				{
					modbus_port_name[valbufpos] = '\0';
					assign(idbuf, modbus_port_name);
					state = 0;
					idbufpos = 0;
					valbufpos = 0;
				}
				else if (is_whitespace(c))
					state = 4;
				else
				{
					modbus_port_name[valbufpos] = c;
					valbufpos++;
					state = 4;
				}
			break;
		}
		c = fgetc(fp);
	}
	valbuf[valbufpos] = '\0';
	assign(idbuf, valbuf);
}

modbus_t *abb_pch550_modbus_init ()
{
	FILE *fp = NULL;
	if (access(CONF_FILE, F_OK) != 0)
	{
		printf("\nWARNING: configuration file %s not found, or\n"
			"insufficient permissions. Default settings will be used,\n"
			"but they are unlikely to work the way you expect. It is\n"
			"recommended that you create one- see sample file\n"
			"'abb.conf' included with the source files.\n\n", CONF_FILE);
	}
	else
	{
		if ((fp = fopen(CONF_FILE, "r")) == NULL)
		{
			fprintf(stderr, "'%s' for reading:\n%s\n",
				CONF_FILE, strerror(errno));
			exit(-1);
		}
		printf("\n");
		parse_conf(fp);
		printf("\n");
	}

	int i;
	pv = malloc(sizeof(element) * modbus_read_count);

	for (i = 0; i < modbus_read_count; i++)
	{
		
		if (fp == NULL)
		{
			strncpy(pv[i].tag, "SENS_DEFAULT", sizeof(pv[i].tag));
			strncpy(pv[i].id, "default_id", sizeof(pv[i].id));
			pv[i].scale = 1;
			pv[i].major = 0;
			pv[i].minor = i;
		}
		else
		{
			pv[i] = get_element(fp);
			pv[i].major = -1;
			pv[i].minor = -1;
		}
	}
		
	if (fp != NULL)
	{
		kvp_conf(fp);
		fclose(fp);	
	}

	printf("%24s : TAG\n", "ID");
	printf("%28s\n", "-----");
	for (i = 0; i < modbus_read_count; i++)
	{
		printf("%24s : %s\n", pv[i].id, pv[i].tag);
	}

	modbus_t *modbusport;

	if (access(modbus_port_name, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", modbus_port_name, strerror(errno));
		exit(-1);
	}

	modbusport = modbus_new_rtu(modbus_port_name, modbus_rtu_baud, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (modbusport == NULL)
	{
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n%s\n",
			modbus_port_name, 
			strerror(errno));
		exit(-1);
	}

	if (modbus_set_slave(modbusport, MB_SLAVE_ADDRESS))
		fail("Failed to set modbus slave address", modbusport);

	if (modbus_connect(modbusport))
		fail("Unable to connect to modbus server", modbusport);


	printf("\n");
	for (i = 0; i < modbus_read_count; i++)
	{
		printf("%16s", pv[i].id);
	}
	printf("\n");
	return modbusport;
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

	n = modbus_read_registers(modbusport, REG_READ_BASE, REG_READ_COUNT, inputs_raw);

	if (n <= 0)
	{
		fail("Unable to read modbus registers", modbusport);
	}

	for (i = 0; i < modbus_read_count; i++)
	{
		pv[i].value_raw = inputs_raw[i];
		pv[i].value_scaled = (float) inputs_raw[i] * pv[i].scale;
	}

	printf("\r");
	for (i = 0; i < modbus_read_count; i++)
		printf("%16.2f", pv[i].value_scaled);
	fflush(stdout);	
}

int posmatch (int maj, int min)
{
	int i;
	for (i = 0; i < modbus_read_count; i++)
	{
		if (pv[i].major == maj &&
			pv[i].minor == min)
		{
			return i;
		}
	}
	return -1;
}

void write_registers_tofile(modbus_t *modbusport)
{
	FILE *fp;
	int i, j;
	char *logfilename = gen_filename(uuid);
	int pathlength = strlen(logfilename) + strlen(SENSORDATA);
	char logpath[pathlength + 1];

	strcpy(logpath, SENSORDATA);
	strcat(logpath, logfilename);
	if ((fp = fopen(logpath, "w")) == NULL)
	{
		fail("Error opening sensor log file for writing register reads", modbusport);
	}

	for (j = 0; j < modbus_read_count; j++)
	{
		int ix = posmatch(j, 0);
		if (ix == -1)
			continue;
		char outstring[512];
		char buf[80];
		strcpy(outstring, "<D>,SEC:PUBLIC");

		for (i = 0; i < modbus_read_count; i++)
		{
			ix = posmatch(j, i);
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


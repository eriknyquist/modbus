#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include "abb_ach550_modbus.h"
#include "abb_ach550_time.h"

#define CONF_FILE "/etc/abb.conf"
#define SENSORDATA "/home/sensordata/"
#define PARAMS 6
#define UUID_LENGTH 37
#define UUID_FILE "/uuid"
#define UPDATE_FREQ_MIN 0.01
#define MB_DATABITS 8
#define MB_STOPBITS 2
#define MB_PARITY 'N'

int paramcount;
double delaytime;
char uuid[38];

modbus_params mbparams = { .rtu_baud=9600,
			   .station_id=0,
			   .read_base=0,
			   .read_count=1,
			   .update_freq_hz=2,
			   .port_name="/dev/null" };
modbus_params *mbp = &mbparams;

element *pv;

static int line = 1;
static int column = 1;

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
	fprintf(stderr, "Syntax error '%c' in configuration file %s, line %d, column %d\n",
		c, CONF_FILE, line, column);
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

void assign (char *param, char *value, modbus_params *mp)
{
	if (strcmp(param, "modbus_rtu_baud") == 0)
		mp->rtu_baud = atoi(value);
	else if (strcmp(param, "modbus_station_id") == 0)
		mp->station_id = atoi(value);
	else if (strcmp(param, "modbus_read_base") == 0)
		mp->read_base = atoi(value);
	else if (strcmp(param, "modbus_read_count") == 0)
		mp->read_count = atoi(value);
	else if (strcmp(param, "update_frequency_hz") == 0)
		mp->update_freq_hz = atof(value);
	else if (strcmp(param, "modbus_port") == 0)
		strncpy(mp->port_name, value, sizeof(mp->port_name));
	else
		nosuchparam(param);

	printf("%24s : %s\n", param, value);
	paramcount++;
}

element get_next_regparam(FILE *fp)
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
		{
			line++;
			column = 1;
		}
		switch (state)
		{
			case 0:
				if (is_id(c))
				{
					e.id[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (c == '#')
					state = 5;
				else if (! is_whitespace(c))
					syntaxerr(c);
			break;
			case 1:
				if (is_id(c))
				{
					e.id[idbufpos] = c;
					idbufpos++;
				}
				else if (c == '{')
				{
					e.id[idbufpos] = '\0';
					state = 2;
				}
				else if (! is_whitespace(c))
					syntaxerr(c);	
			break;
			case 2:
				if (is_id(c))
				{
					pbuf[pbufpos] = c;
					pbufpos++;
				}
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
				else if (! is_whitespace(c))
					syntaxerr(c);	
			break;
			case 3:
				if (is_id(c))
				{
					e.tag[tagpos] = c;
					tagpos++;
				}
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
				else if (! is_whitespace(c))
					syntaxerr(c);
			break;
			case 4:
				if (is_dec(c))
				{
					scale[scalepos] = c;
					scalepos++;
				}
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
		column++;
	}
	e.scale = atof(scale);
	return e;
}

int idcmp (char *idc, element *v, modbus_params *mbp)
{
	int i;
	for (i = 0; i < mbp->read_count; i++)
		{
		if (strcmp(idc, v[i].id) == 0)
			return i;
	}
	return -1;
}

void parse_order (FILE *fp, element *v, modbus_params *mbp)
{
	int majc = 0, minc = 0;
	char idbuf[80], c;
	int idbufpos = 0;
	uint8_t state = 0;
	while ((c = fgetc(fp)) != EOF)
	{
		if (c == '\n')
		{
			line++;
			column = 1;
		}
		switch (state)
		{
			case 0:
				if (c == '{')
					state = 1;
				else if (c == '#')
					state = 2;
				else if (! is_whitespace(c))
					syntaxerr(c);
			break;
			case 1:
				if (is_id(c))
				{
					idbuf[idbufpos] = c;
					idbufpos++;
				}
				else if (c == ',' || c == '}')
				{
					idbuf[idbufpos] = '\0';
					int ti = idcmp(idbuf, v, mbp);
					if (ti == -1)
						unrec_id(idbuf);
					if (v[ti].major != -1)
						doubleassn(idbuf);
					v[ti].major = majc;
					v[ti].minor = minc;
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
				else if (! is_whitespace(c))
					syntaxerr(c);
			break;
			case 2:
				if (c == '\n')
					state = 0;
			break;
		}
		column++;
	}
}

void parse_modbus_params(FILE *fp, modbus_params *mp)
{
	char c;
	uint8_t isfloat = 0, state = 0, idbufpos = 0, valbufpos = 0;
	char idbuf[80], valbuf[32];

	c = fgetc(fp);
	while (c != ';')
	{
		if (c == '\n')
		{
			line++;
			column = 1;
		}
		switch (state)
		{
			case 0:
				if (is_id(c))
				{
					idbuf[idbufpos] = c;
					idbufpos++;
					state = 1;
				}
				else if (c == '#')
					state = 3;
				else if (! is_whitespace(c))
					syntaxerr(c);
			break;
			case 1:
				if (is_id(c))
				{
					idbuf[idbufpos] = c;
					idbufpos++;
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
					if (c == '.') isfloat = 1;
				}
				else if (c == ',')
				{
					valbuf[valbufpos] = '\0';
					assign(idbuf, valbuf, mp);
					state = 0;
					idbufpos = 0;
					valbufpos = 0;
				}
				else if (! is_whitespace(c))
					syntaxerr(c);
			break;
			case 3:
				if (c == '\n')
					state = 0;
			break;
			case 4:
				if (c == ',')
				{
					mp->port_name[valbufpos] = '\0';
					assign(idbuf, mp->port_name, mp);
					state = 0;
					idbufpos = 0;
					valbufpos = 0;
				}
				else if (! is_whitespace(c))
				{
					mp->port_name[valbufpos] = c;
					valbufpos++;
				}
			break;
		}
		column++;
		c = fgetc(fp);
	}
	valbuf[valbufpos] = '\0';
	assign(idbuf, valbuf, mp);
}

modbus_t *abb_ach550_modbus_init ()
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
		parse_modbus_params(fp, mbp);
		printf("\n");
	}

	if (((int) floorf(mbp->update_freq_hz)) * 1000 <
		((int) floorf(UPDATE_FREQ_MIN)) * 1000)
	{
		fprintf(stderr, "Error in configuration file '%s' : parameter \n"
			"'update_frequency_hz' must be set to %.2f or higher.\n"
			"You have entered a value of %ld\n",
			CONF_FILE, UPDATE_FREQ_MIN, mbp->update_freq_hz);
		exit(-1);
	}


	int i;
	pv = malloc(sizeof(element) * mbp->read_count);
	for (i = 0; i < mbp->read_count; i++)
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
			pv[i] = get_next_regparam(fp);
			pv[i].major = -1;
			pv[i].minor = -1;
		}
	}
		
	if (fp != NULL)
	{
		parse_order(fp, pv, mbp);
		fclose(fp);	
	}

	printf("%24s : TAG\n", "ID");
	printf("%28s\n", "-----");
	for (i = 0; i < mbp->read_count; i++)
	{
		printf("%24s : %s\n", pv[i].id, pv[i].tag);
	}

	modbus_t *modbusport;

#ifndef NOMODBUS
	if (access(mbp->port_name, F_OK) != 0)
	{
		printf("Error accessing '%s':\n%s\n", mbp->port_name, strerror(errno));
		exit(-1);
	}

	modbusport = modbus_new_rtu(mbp->port_name, mbp->rtu_baud, MB_PARITY, MB_DATABITS, MB_STOPBITS);

	if (modbusport == NULL)
	{
		fprintf(stderr, "Unable to create the libmodbus context on serial port %s\n%s\n",
			mbp->port_name, 
			strerror(errno));
		exit(-1);
	}

	if (modbus_set_slave(modbusport, mbp->station_id))
		fail("Failed to set modbus slave address", modbusport);

	if (modbus_connect(modbusport))
		fail("Unable to connect to modbus server", modbusport);
#endif /* NOMODBUS */

	printf("\n");
	for (i = 0; i < mbp->read_count; i++)
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

int abb_ach550_read (uint16_t *inputs_raw, modbus_t *modbusport)
{
	int n, i;

#ifndef NOMODBUS
	n = modbus_read_registers(modbusport, mbp->read_base, mbp->read_count, inputs_raw);
	if (n <= 0)
	{
		fail("Unable to read modbus registers", modbusport);
	}
#endif

	for (i = 0; i < mbp->read_count; i++)
	{
		pv[i].value_raw = inputs_raw[i];
		pv[i].value_scaled = (float) inputs_raw[i] * pv[i].scale;
	}

	printf("\r");
	for (i = 0; i < mbp->read_count; i++)
		printf("%16.2f", pv[i].value_scaled);
	fflush(stdout);	
}

int posmatch (int maj, int min)
{
	int i;
	for (i = 0; i < mbp->read_count; i++)
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
		fail("Error opening sensor log file for writing register reads", NULL);
	}

	for (j = 0; j < mbp->read_count; j++)
	{
		int ix = posmatch(j, 0);
		if (ix == -1)
			continue;
		char outstring[512];
		char buf[80];
		strcpy(outstring, "<D>,SEC:PUBLIC");

		for (i = 0; i < mbp->read_count; i++)
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


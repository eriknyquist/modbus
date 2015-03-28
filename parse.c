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

int paramcount;
double delaytime;
char uuid[38];

static int line = 1;
static int column = 1;

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

void assign (char *param, char *value, modbusport *mp)
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

int idcmp (char *idc, element *v, modbusport *mp)
{
	int i;
	for (i = 0; i < mp->read_count; i++)
		{
		if (strcmp(idc, v[i].id) == 0)
			return i;
	}
	return -1;
}

void parse_order (FILE *fp, element *v, modbusport *mp)
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
					int ti = idcmp(idbuf, v, mp);
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

void parse_modbus_params(FILE *fp, modbusport *mp)
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

#ifndef INIT_H_
#define INIT_H_

typedef struct modbusport
{
	modbus_t *port;
	char port_name[128];
	int rtu_baud;
	int station_id;
	int read_base;
	int read_count;
	time_t secs;
	char uuid[38];
	char dname[32];
	char logfile[256];
	FILE *logfp;
	FILE *errfp;
	unsigned long pid;
} modbusport;

typedef struct element
{
	uint16_t value_raw;
	float value_scaled;
	char tag[80];
	char id[80];
	float scale;
	int major;
	int minor;
} element;

modbus_t *modbus_init (modbusport *mp, element *pv);
void ile_aip_init (modbusport *mp);
void get_modbus_params(modbusport *mp);

#endif

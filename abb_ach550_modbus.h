#ifndef ABB_ACH550_MODBUS_H_
#define ABB_ACH550_MODBUS_H_


typedef struct modbusport
{
	modbus_t *port;
	char port_name[128];
	int rtu_baud;
	int station_id;
	int read_base;
	int read_count;
	float update_freq_hz;
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

modbus_t *abb_ach550_modbus_init (modbusport *mp);
void ile_aip_init (void);
int abb_ach550_read (uint16_t *inputs_raw, modbusport *mp);
void write_registers_tofile (modbusport *mp);
void fail (char *errstr, modbus_t *mp);

#endif

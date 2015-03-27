#ifndef ABB_ACH550_MODBUS_H_
#define ABB_ACH550_MODBUS_H_

modbus_t *abb_ach550_modbus_init ();
void ile_aip_init (void);
int abb_ach550_read (uint16_t *inputs_raw, modbus_t *modbusport);
void write_registers_tofile (modbus_t *modbusport);
void fail (char *errstr, modbus_t *modbusport);

typedef struct modbus_params
{
	int rtu_baud;
	int station_id;
	int read_base;
	int read_count;
	float update_freq_hz;
	char port_name[128];
} modbus_params;

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

#endif

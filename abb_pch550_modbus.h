#ifndef ABB_PCH550_MODBUS_H_
#define ABB_PCH550_MODBUS_H_

modbus_t *abb_pch550_modbus_init (char *serialport);
void ile_aip_init (void);
int abb_pch550_read (uint16_t *inputs_raw, modbus_t *modbusport);
void write_registers_tofile (modbus_t *modbusport);
void fail (char *errstr, modbus_t *modbusport);
void parse_conf (void);

typedef struct element
{
	uint16_t value_raw;
	float value_scaled;
	char *tag;
	char *id;
	float scale;
} element;

#endif

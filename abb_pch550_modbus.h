#ifndef ABB_PCH550_MODBUS_H_
#define ABB_PCH550_MODBUS_H_

modbus_t *abb_pch550_modbus_init(char *serialport);
int abb_pch550_read (uint16_t *inputs_raw, modbus_t *modbusport);
void write_registers_tofile(char *filename, modbus_t *modbusport);
void fail (char *errstr, modbus_t *modbusport);

typedef struct element
{
	uint16_t value_raw;
	float value_scaled;
	char *desc;
} element;

#endif

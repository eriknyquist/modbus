#ifndef ABB_PCH550_MODBUS_H_
#define ABB_PCH550_MODBUS_H_

modbus_t *abb_pch550_modbus_init(char *serialport);
int abb_pch550_read (uint16_t *inputs_raw, modbus_t *modbusport);
void fail (char *errstr, modbus_t *modbusport);

#endif

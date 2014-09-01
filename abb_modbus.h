#ifndef ABB_MODBUS_H_
#define ABB_MODBUS_H_

modbus_t *abb_modbus_init(char *serialport);
int abb_update_input_registers (uint16_t *inputs_raw, float *inputs_scaled, modbus_t *modbusport);
void fail (char *errstr, modbus_t *modbusport);

#endif

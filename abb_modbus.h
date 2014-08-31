#ifndef ABB_MODBUS_H_
#define ABB_MODBUS_H_

double getms();
modbus_t *abb_modbus_init(char *serialport);
void fail (modbus_t *mbp);
void update (uint16_t *inputs_raw, float *inputs_scaled, modbus_t *mbp);
int abb_update_input_registers (uint16_t *inputs_raw, float *inputs_scaled, modbus_t *modbusport);

#endif

#ifndef ABB_MODBUS_H_
#define ABB_MODBUS_H_

double getms();
void abb_modbus_init(char *serialport, modbus_t *modbusport);
void fail (modbus_t *mbp);
void update (uint16_t *inputs_raw, float *inputs_scaled, modbus_t *mbp);

#endif

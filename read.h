#ifndef READ_H_
#define READ_H_

int abb_ach550_read (uint16_t *inputs_raw, modbusport *mp, element *pv);
void write_registers_tofile (modbusport *mp, element *pv);

#endif

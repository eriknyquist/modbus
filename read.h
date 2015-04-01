#ifndef READ_H_
#define READ_H_

int mbd_read (modbusport *mp, element *pv);
void write_registers_tofile (modbusport *mp, element *pv);

#endif

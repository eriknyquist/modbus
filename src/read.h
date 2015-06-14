#ifndef READ_H_
#define READ_H_

void mbd_read (modbusport *mp, element *pv, logging *lp);
void write_registers_tofile (modbusport *mp, element *pv, logging *lp);

#endif

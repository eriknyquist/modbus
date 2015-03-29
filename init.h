#ifndef INIT_H_
#define INIT_H_

#include "shared.h"

modbus_t *modbus_init (modbusport *mp, element *pv);
void ile_aip_init (modbusport *mp);
void get_modbus_params(modbusport *mp);

#endif

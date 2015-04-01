#ifndef INIT_H_
#define INIT_H_

#include "shared.h"

element *mbd_init(modbusport *mp);
void mbd_exit(modbusport *mp);

#endif

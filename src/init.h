#ifndef INIT_H_
#define INIT_H_

#include "shared.h"

element *mbd_init(modbusport *mp, logging *lp);
void mbd_exit(modbusport *mp, logging *lp);

#endif

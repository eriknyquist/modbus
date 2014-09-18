#include <stdio.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "abb_pch550_modbus.h"
#include "abb_pch550_time.h"
#include "common.h"

int write_register_tofile (FILE *fp, char *timestamp_string, element *pv)
{	
	if (fprintf(fp, "<D>,SEC:PUBLIC,%s:%.2f,timestamp=\"%s\"\n", 
		pv->desc, pv->value_scaled, timestamp_string) < 0)
		return -1;
	return 0;
}

#include <stdio.h>
#include <modbus.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "abb_pch550_modbus.h"
#include "abb_pch550_time.h"
#include "common.h"

void write_keyvalue_pairs (char *filename, element *pv[])
{
	FILE *fp;
	if ((fp = fopen(filename, "w+")) == NULL)
	{
		fprintf(stderr, "Error opening file %s for writing\n%s\n",
			filename, strerror(errno));
		exit(-1);
	}
	
	int i;

	fputc('<', fp);
	for (i = 0; i < REG_READ_COUNT; i++)
	{
		fprintf(fp, "%s=%.1f", pv[i]->desc, pv[i]->value_scaled);
		if (i < (REG_READ_COUNT - 1))
		{
			fputc(',', fp);
		}
		fputc(' ', fp);
	}
	fprintf(fp, "timestamp='%s'>", timestamp());
	fclose(fp);
}

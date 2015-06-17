/* Modbus logging daemon
 * Copyright (C) 2015 Erik Nyquist <eknyquist@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <modbus.h>
#include <sys/types.h>
#include "shared.h"

void usage(char *arg0)
{
	printf("Usage: %s [-v]\n\n", arg0);
	printf("-v   verbose- record all modbus activity in logfile\n");
	exit(EINVAL);
}

int parse_arg(char *arg, logging *lp)
{
	int numchars;
	int ret = 0;
	int i;

	if (arg == NULL || arg[0] !=  '-')
		return ret;

	numchars = strlen(arg);

	for (i = 1; i < numchars; i++) {
		if (arg[i] == 'v'){
			lp->verbose = 1;
			ret = 1;
		} else {
			ret = -1;
			break;
		}
	}

	return ret;
}

void parse_args(int argc, char *argv[], logging *lp)
{
	int argpos;
	int ret;

	argpos = 1;

	while (argpos < argc) {
		ret = parse_arg(argv[argpos], lp);

		if (ret < 1)
			usage(argv[0]);
		else
			argpos += ret;
	}
}

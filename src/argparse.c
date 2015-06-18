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
	printf("\nUsage: %s [-vq -c <file>]\n\n", arg0);
	printf("-v         verbose   - record all modbus activity in logfiles\n");
	printf("                       (default is to log start/stop times, configuration\n");
	printf("                       data and errors.)\n");
	printf("-q         quiet     - don't create any logfiles or print to stdout\n");
	printf("                       (with the exception of sensor logfiles)\n");
	printf("-c <file>  conf file - read configuration from <file>\n");
	printf("                       (default is " DEFAULT_CONF_FILE ")\n");
	printf("\n");
	exit(EINVAL);
}

int parse_arg(int pos, int argc, char *argv[], logging *lp)
{
	char *arg;
	int numchars;
	int ret = 0;
	int i;

	arg = argv[pos];

	if (arg == NULL || strlen(arg) < 2 || arg[0] !=  '-')
		return ret;

	numchars = strlen(arg);

	for (i = 1; i < numchars; i++) {
		if (arg[i] == 'v') {
			lp->verbosity = LOG_VERBOSE;
			ret = 1;
		} else if (arg[i] == 'q') {
			lp->verbosity = LOG_QUIET;
			ret = 1;
		} else if (arg[i] == 'c') {
			if (numchars > 2 || argc < (pos + 2)) {
				ret = 0;
				break;
			}

			strncpy(lp->conffile, argv[pos + 1], sizeof(lp->conffile));
			ret = 2;
		} else {
			ret = 0;
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
		ret = parse_arg(argpos, argc, argv, lp);

		if (ret < 1)
			usage(argv[0]);
		else
			argpos += ret;
	}
}

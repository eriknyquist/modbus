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
#include <unistd.h>
#include <errno.h>
#include <modbus.h>
#include <sys/types.h>
#include "shared.h"

void usage(char *arg0)
{
	printf("\nUsage: %s [-vqmfh -c <file>]\n\n", arg0);
	printf("-v         verbose    - Record all modbus activity in logfiles\n");
	printf("                        (default is to log start/stop times,\n");
	printf("                        configuration data and errors.)\n\n");
	printf("-q         quiet      - Don't create any logfiles or print to stdout\n");
	printf("                        (with the exception of sensor logfiles)\n\n");
	printf("-m         monitor    - Don't log any daemon activity or modbus reads,\n");
	printf("                        stay in the foreground, and dump all modbus\n");
	printf("                        register data to stdout\n\n");
	printf("-c <file>  conf file  - Read configuration from <file>\n");
	printf("                        (default is " DEFAULT_CONF_FILE ")\n\n");
	printf("-f         don't fork - Don't run as a forked child process.\n");
	printf("                        (program stays in the foreground, and can be\n");
	printf("                        killed with Ctrl-C)\n\n");
	printf("-h         help       - Show this guide\n\n");

	printf("Copyright (C) 2015 Erik Nyquist\n\n");
	printf("This program is free software: you can redistribute it and/or modify\n");
	printf("it under the terms of the GNU General Public License as published by\n");
	printf("the Free Software Foundation, either version 3 of the License, or\n");
	printf("at your option) any later version.\n\n");
	printf("This program is distributed in the hope that it will be useful,\n");
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	printf("GNU General Public License for more details.\n\n");
 	printf("You should have received a copy of the GNU General Public License\n");
	printf("along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n");
	exit(EINVAL);
}

int file_accessible(char *fn)
{
	int ret = 0;

	if (access(fn, F_OK) != 0)
		ret = -1;

	return ret;
}

int parse_arg(int pos, int argc, char *argv[], logging *lp, mbdinfo *mip)
{
	char *arg;
	int numchars;
	int ret = 0;
	int i;

	arg = argv[pos];

	if (arg == NULL || strlen(arg) < 2 || arg[0] !=  '-')
		return ret;

	numchars = (int) strlen(arg);

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

			strncpy(mip->conffile, argv[pos + 1],
			        sizeof(mip->conffile));

			
			if (file_accessible(mip->conffile) < 0) {
				printf("%s: %s\n", mip->conffile,
				       strerror(ENOENT));
				ret = -ENOENT;
			} else {
				ret = 2;
			}
		} else if (arg[i] == 'f') {
			mip->shouldfork = 0;
			ret = 1;
		} else if (arg[i] == 'm') {
			mip->shouldfork = 0;
			lp->verbosity = LOG_QUIET;
			mip->monitor = 1;
			ret = 1;
		} else if (arg[i] == 'h') {
			usage(argv[0]);
		} else {
			printf("Unrecognised option '%c'\n", arg[i]);
			ret = 0;
			break;
		}
	}

	return ret;
}

void parse_args(int argc, char *argv[], logging *lp, mbdinfo *mip)
{
	int argpos;
	int ret;

	argpos = 1;

	while (argpos < argc) {
		ret = parse_arg(argpos, argc, argv, lp, mip);

		if (ret < 0)
			exit(ret);
		else if (ret < 1)
			usage(argv[0]);			
		else
			argpos += ret;
	}
}

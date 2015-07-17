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

#ifndef TIME_H_
#define TIME_H_

#define TIMESTAMP_LEN 25

struct itimerspec;

char *gen_filename(char *uuid);
int start_periodic_task(unsigned long msecs, void (*task));
char *timestamp();

/* For unit testing */

char *gen_filename (char *uuid);
void ms_to_itimerspec(struct itimerspec *tp, unsigned long msecs);

#endif

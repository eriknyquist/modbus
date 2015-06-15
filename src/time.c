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
#include <modbus.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <time.h>
#include <string.h>
#include "init.h"
#include "time.h"

#define CLOCKID CLOCK_MONOTONIC

char *gen_filename (char *uuid)
{
	/* generates the file name for logged register reads, 
 	 * which is comprised of a timestamp & the system's UUID */
	char timestamp[40];
	char *filename = malloc(80);
        struct timeval tv;

        gettimeofday (&tv, NULL);

	snprintf(timestamp, sizeof(timestamp), "%ld%ld-", tv.tv_sec, tv.tv_usec / 1000);
	strcpy(filename, timestamp);
	strcat(filename, uuid);
	strcat(filename, ".log");

        return filename;
}

void ms_to_itimerspec(struct itimerspec *tp, unsigned long msecs)
{
	if (msecs < 1000) {
		tp->it_value.tv_nsec = 
		tp->it_interval.tv_nsec = 
		msecs * 1000000L;

		tp->it_value.tv_sec = 0;
		tp->it_interval.tv_sec = 0;
	} else {
		tp->it_value.tv_nsec = tp->it_interval.tv_nsec =
		(msecs % 1000L) * 1000000L;

		tp->it_value.tv_sec = tp->it_interval.tv_sec = msecs / 1000L;
	}
}

int create_periodic(unsigned long msecs, void (*thread))
{
	/* sets up the function pointed to by 'thread'
  	 * to run every 'period' seconds, via a new thread. */
        int status;
	timer_t timer_id;
        struct itimerspec ts, *tp;
        struct sigevent se;

	tp = &ts;

        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &timer_id;
        se.sigev_notify_function = thread;
        se.sigev_notify_attributes = NULL;

	ms_to_itimerspec(tp, msecs);
	
        status = timer_create(CLOCK_MONOTONIC, &se, &timer_id);
        if (status == -1)
                return status;

        status = timer_settime(timer_id, 0, tp, 0);

	return status;
}

char *timestamp()
{
	/* plain-text calendar timestamp.
 	 * used for logging. */
	time_t rawtime;
	struct tm *info;

	time(&rawtime);

	info = localtime(&rawtime);
	char *tmp = asctime(info);
	tmp[strlen(tmp) - 1] = 0;
	return tmp;
}

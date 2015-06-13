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

int create_periodic(time_t period, void (*thread))
{
	/* sets up the function pointed to by 'thread'
  	 * to run every 'period' seconds, via a new thread. */
        int status;
	timer_t timer_id;
        struct itimerspec ts;
        struct sigevent se;

        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &timer_id;
        se.sigev_notify_function = thread;
        se.sigev_notify_attributes = NULL;

        ts.it_value.tv_sec = period;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = period;
        ts.it_interval.tv_nsec = 0;

        status = timer_create(CLOCK_MONOTONIC, &se, &timer_id);
        if (status == -1)
                return status;

        status = timer_settime(timer_id, 0, &ts, 0);

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

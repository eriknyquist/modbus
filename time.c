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

uint64_t getms ()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return (uint64_t) (time.tv_sec * 1000000) + time.tv_usec;
}

char *gen_filename (char *uuid)
{
	char timestamp[40];
	char *filename = malloc(80);
        struct timeval tv;

        gettimeofday (&tv, NULL);

	snprintf(timestamp, sizeof(timestamp), "%d%ld-", tv.tv_sec, tv.tv_usec / 1000);
	strcpy(filename, timestamp);
	strcat(filename, uuid);
	strcat(filename, ".log");

        return filename;
}

int create_periodic(time_t period, void (*thread))
{
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
        if (status == -1)
                return status;
	return status;
}

int timestamp(char *ts)
{
   time_t rawtime;
   struct tm *info;

   time(&rawtime);

   info = localtime(&rawtime);
   snprintf(ts, TIMESTAMP_LEN, "%02d:%02d:%02d", info->tm_hour, info->tm_min, info->tm_sec);
}

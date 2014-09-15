#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include "abb_pch550_modbus.h"
#include "common.h"

#define CLOCKID CLOCK_MONOTONIC

char * timestamp ()
{
        char *buf = malloc(22);
        struct timeval tv;
        struct timezone tz;
        struct tm *tm;

        gettimeofday (&tv, &tz);
        tm = localtime(&tv.tv_sec);
        int milliseconds = (tm->tm_hour * 3600 * 1000 + tm->tm_min * 60 * 1000 +
        tm->tm_sec * 1000 + tv.tv_usec / 1000) % 1000;
        snprintf(buf, 22, "%d/%d/%d-%02d:%02d:%02d.%02d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                tm->tm_hour, tm->tm_min, tm->tm_sec, milliseconds);

        return buf;
}

void start_interval_timer(long long freq_nanosecs, int sig)
{
	timer_t timerid;	
	struct sigevent sev;
	struct itimerspec its;	

	/* Create the timer */

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = sig;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		fail("Error creating timer", NULL);

	/* Start the timer */

	its.it_value.tv_sec = freq_nanosecs / 1000000000;
	its.it_value.tv_nsec = freq_nanosecs % 1000000000;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;

	if (timer_settime(timerid, 0, &its, NULL) == -1)
		fail("Error starting timer", NULL);
}

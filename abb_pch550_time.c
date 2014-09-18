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

long long getms ()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return (long long) (time.tv_sec + (time.tv_usec / 1000000.0)) * 1000.0;
}

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

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

double getms ()
{
        struct timeval time;
        gettimeofday (&time, NULL);
        return (time.tv_sec + (time.tv_usec / 1000000.0)) * 1000.0;
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
        snprintf(buf, 22, "%d/%d/%d-%d:%d:%d.%d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                tm->tm_hour, tm->tm_min, tm->tm_sec, milliseconds);

        return buf;
}

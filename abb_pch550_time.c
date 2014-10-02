#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "abb_pch550_modbus.h"

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

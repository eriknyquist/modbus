#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)
#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGUSR1

uint8_t gotsig = 0; 

static void handler(int signum)
{
	gotsig = 1; 
}

void start_timer(long long freq_nanosecs)
{
	timer_t timerid;	
	struct sigevent sev;
	struct itimerspec its;	
        struct sigaction sa;

	/* Establish handler for timer signal */

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	if (sigaction(SIG, &sa, NULL) == -1)
		errExit("sigaction");

	/* Create the timer */

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		errExit("timer_create");

	/* Start the timer */

	its.it_value.tv_sec = freq_nanosecs / 1000000000;
	its.it_value.tv_nsec = freq_nanosecs % 1000000000;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;

	if (timer_settime(timerid, 0, &its, NULL) == -1)
		errExit("timer_settime");
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <freq-nanosecs>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	start_timer(atoll(argv[1]));
	while (1)
	{
		if (gotsig)
		{
			printf("c\n");
			gotsig = 0;
		}
	}
}

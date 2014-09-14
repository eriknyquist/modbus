#ifndef ABB_TIME_H_
#define ABB_TIME_H_

char * timestamp(void);
void start_interval_timer(long long freq_nanosecs, int sig);

#endif

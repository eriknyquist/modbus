#ifndef TIME_H_
#define TIME_H_

char *gen_filename(char *uuid);
int create_periodic(time_t period, void (*thread));
int timestamp(char *ts);

#endif

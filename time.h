#ifndef TIME_H_
#define TIME_H_

#define TIMESTAMP_LEN 25

char *gen_filename(char *uuid);
int create_periodic(time_t period, void (*thread));
char *timestamp();

#endif

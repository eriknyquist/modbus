#ifndef LOG_H_
#define LOG_H_

void fatal (char *errstr, modbusport *mp);
void err (char *errstr, modbusport *mp);
void logger (char *str, modbusport *mp);
void log_init(modbusport *mp);

#endif

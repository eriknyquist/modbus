#ifndef LOG_H_
#define LOG_H_

void fatal (char *errstr, modbusport *mp, logging *lp);
void err (char *errstr, logging *lp);
void logger (char *str, logging *lp);
void log_init (logging *lp);

#endif

#ifndef COMMON_H_
#define COMMON_H_

/* signal to use for timer */
#define SIG SIGUSR1

/* Frequency at which the input
   registers will be read */
#define UPDATE_FREQUENCY_HZ 10

#define FREQ_RESOLUTION_HZ 0.1
#define CURRENT_RESOLUTION_A 0.1
#define VOLTAGE_RESOLUTION_V 1
#define MOTORSPEED_RESOLUTION_RPM 1
#define KW_RESOLUTION 0.1
#define KWH_RESOLUTION 1

/* modbus input register, start of address space to read */
#define REG_READ_BASE 4

/* number of registers to read */
#define REG_READ_COUNT 6

#define BANNER \
"\n         \
Freq.(Hz)      \
Current(A)      \
Voltage(V)      \
Motor Speed(RPM)      \
KW              \
KWh\n"

#endif

#ifndef COMMON_H_
#define COMMON_H_


/* Frequency at which the input
   registers will be read */
#define UPDATE_FREQUENCY_HZ 10

#define FREQ_RESOLUTION_HZ 0.1
#define CURRENT_RESOLUTION_A 0.1
#define VOLTAGE_RESOLUTION_V 1
#define MOTORSPEED_RESOLUTION_RPM 1

/* modbus input register, start of address space to read */
#define INPUT_REG_READ_BASE 5

/* number of registers to read */
#define INPUT_REG_READ_COUNT 4

#define BANNER \
"         \
Freq.(Hz)       \
Current(A)      \
Voltage(V)      \
Motor Speed(RPM)\n"

#endif

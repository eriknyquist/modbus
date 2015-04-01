#ifndef SHARED_H_
#define SHARED_H_

#define SENSORDATA "/home/sensordata"
#define UUID_LENGTH 37
#define UUID_FILE "/uuid"
#define CONF_FILE "/etc/abb.conf"
#define LOGDIR "/home/root"
#define MAX_LOG_LEN 256
#define MAX_PATH_LEN 256

typedef struct modbusport
{
        modbus_t *port;
        char port_name[128];
        int rtu_baud;
        int station_id;
        int read_base;
        int read_count;
        time_t secs;

	/* TODO: this stuff should probably
 	 * be inside a seperate struct for
 	 * daemon-logging related things.*/
        char uuid[38];
        char dname[32];
	char logdir[MAX_PATH_LEN];
        char logfile[MAX_PATH_LEN];
        char errfile[MAX_PATH_LEN];
        unsigned long pid;
} modbusport;

typedef struct element
{
        uint16_t value_raw;
        float value_scaled;
        char tag[80];
        char id[80];
        float scale;
        int major;
        int minor;
} element;

#endif

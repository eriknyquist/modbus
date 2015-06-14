#ifndef SHARED_H_
#define SHARED_H_

#define DEFAULT_PORT_NAME     "/dev/null"
#define CONF_FILE             "/etc/abb.conf"
#define DEFAULT_SENS_LOGDIR   "/home/sensordata"
#define DEFAULT_LOGDIR        "/home/root"
#define DEFAULT_UUID_FILE     "/uuid"
#define UUID_LENGTH           37
#define MAX_LOG_LEN           256
#define MAX_PATH_LEN          256
#define DEFAULT_BAUD          9600
#define DEFAULT_STATION_ID    0
#define DEFAULT_READ_BASE     0
#define DEFAULT_READ_COUNT    1
#define DEFAULT_SECS          2

typedef struct modbusport
{
        modbus_t *port;
        char port_name[128];
        int rtu_baud;
        int station_id;
        int read_base;
        int read_count;
        time_t secs;

} modbusport;

typedef struct logging
{
        char uuid[38];
        char dname[32];
	char uuidfile[MAX_PATH_LEN];
	char sens_logdir[MAX_PATH_LEN];
	char logdir[MAX_PATH_LEN];
        char logfile[MAX_PATH_LEN];
        char errfile[MAX_PATH_LEN];
        unsigned long pid;
} logging;

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

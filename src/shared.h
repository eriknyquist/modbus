/* Modbus logging daemon
 * Copyright (C) 2015 Erik Nyquist <eknyquist@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHARED_H_
#define SHARED_H_

#define LOG_QUIET             0
#define LOG_NORMAL            1
#define LOG_VERBOSE           2

#define DEFAULT_PORT_NAME     "/dev/null"
#define CONF_FILE             "/etc/mbd.conf"
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
#define DEFAULT_MSECS         2000

#define DEFAULT_VERBOSITY     LOG_NORMAL

typedef struct modbusport
{
        modbus_t *port;
        char port_name[128];
        int rtu_baud;
        int station_id;
        int read_base;
        int read_count;
        unsigned long msecs;
	unsigned long long readcount;

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
	short verbosity;
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

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <modbus.h>
#include <limits.h>
#include "shared.h"
#include "confparse.h"
#include "log.h"

#define RUN_OFF1_MASK             0x0001
#define RUN_OFF2_MASK             0x0002
#define RUN_OFF3_MASK             0x0004
#define ENABLE_OP_MASK            0x0008
#define RAMP_NORMAL_MASK          0x0010
#define ENABLE_RAMP_MASK          0x0020
#define ENABLE_RFG_MASK           0x0040
#define FAULT_RESET_MASK          0x0080
#define FIELDBUS_ENABLED_MASK     0x0400
#define EXT2_SELECT_MASK          0x0800

#define START_TOKEN               "start"
#define STOP_TOKEN                "stop"

#define ABB_CMD_WORD_ADDR         0
#define ABB_SPEED_CTL_ADDR        2

uint16_t init_word =              RUN_OFF2_MASK | RUN_OFF3_MASK;

uint16_t stop_word =              RUN_OFF1_MASK | RUN_OFF2_MASK | RUN_OFF3_MASK
                                  | ENABLE_RAMP_MASK | ENABLE_RFG_MASK;

uint16_t start_word =             RUN_OFF1_MASK | RUN_OFF2_MASK | RUN_OFF3_MASK
                                  | ENABLE_OP_MASK | ENABLE_RAMP_MASK |
                                  ENABLE_RFG_MASK;

int init_drive_ready (mbdport *mp, mbdinfo *mip, logging *lp)
{
	int ret = 0;
#ifndef NOMODBUS
	int mret;

	/* Enter 'ready to switch on' state */
	mret = modbus_write_register(mp->port, ABB_CMD_WORD_ADDR, init_word);

	/* ACH550 fieldbus manual says to wait at least 100ms before proceeding,
	 * we'll give it 150 to be safe (EN_ACH550_EFB_D, page 34) */
	usleep(150000);

	/* Enter 'operating' state */
	mret = modbus_write_register(mp->port, ABB_CMD_WORD_ADDR, start_word);

	if (mret < 0) {
		ret = errno;
		err("Modbus error initialising drive operation", lp, mip, ret);
	}
#endif

	return ret;
}

int write_cmd (mbdport *mp, mbdinfo *mip, logging *lp, uint16_t word)
{
	int ret;
	int status = 0;

#ifndef NOMODBUS
	pthread_mutex_lock(&mp->lock);
	status = modbus_write_register(mp->port, ABB_CMD_WORD_ADDR, word);
	pthread_mutex_unlock(&mp->lock);
#endif

	if (status < 0) {
		err("Error writing modbus register", lp, mip, errno);
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

int write_speed (mbdport *mp, mbdinfo *mip, logging *lp, char *input)
{
	int ret;
	int status = 0;
	uint16_t speed;
	unsigned long raw;

	errno = 0;
	raw = strtoul(input, NULL, 10);

	if (errno != 0 || raw > UINT16_MAX) {
		ret = -1;
	} else {
		speed = (uint16_t) raw;

#ifndef NOMODBUS
		pthread_mutex_lock(&mp->lock);
		status = modbus_write_register(mp->port, ABB_SPEED_CTL_ADDR,
		                               speed);
		pthread_mutex_unlock(&mp->lock);
#endif

		if (status < 0) {
			err("modbus error while setting drive speed\n", lp, mip,
			    errno);
		}

		ret = 0;
	}

	return ret;
}

int perform_action (mbdport *mp, mbdinfo *mip, logging *lp, char *cmd)
{
	char *msg = NULL;
	int ret;

	if (strcmp(cmd, START_TOKEN) == 0) {
		ret = write_cmd(mp, mip, lp, start_word);
		msg = "start signal received";
	} else if (strcmp(cmd, STOP_TOKEN) == 0) {
		ret = write_cmd(mp, mip, lp, stop_word);
		msg = "stop signal received";
	} else if (only_has_digits(cmd) == 1) {
		ret = write_speed(mp, mip, lp, cmd);
		msg = "speed change received";
	} else {
		msg = "Invalid command written to " CONTROL_FIFO_PATH;
		ret = -1;
	}

	if (msg != NULL && lp->verbosity != LOG_QUIET)
		logger(msg, lp, mip);

	return ret;
}

void send_ctrl_msg (mbdport *mp, mbdinfo *mip, logging *lp)
{
	size_t n;
	char buf[80];
	int fd;

	fd = open(mip->fifo, O_RDONLY);
	n = read(fd, &buf, sizeof(buf) - 1);
	close(fd);

	buf[n] = '\0';

	perform_action(mp, mip, lp, buf);
}

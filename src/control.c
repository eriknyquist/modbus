#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <modbus.h>
#include "shared.h"
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

#define CMD_WORD_ADDR             0

int init_drive_ready (mbdport *mp, mbdinfo *mip, logging *lp)
{
	int ret = 0;
#ifndef NOMODBUS
	int mret;
	uint16_t init_ctl_word = 0x000;

	init_ctl_word = RUN_OFF2_MASK | RUN_OFF3_MASK | ENABLE_OP_MASK |
	                RAMP_NORMAL_MASK | ENABLE_RAMP_MASK | ENABLE_RFG_MASK |
	                FIELDBUS_ENABLED_MASK | EXT2_SELECT_MASK;

	mret = modbus_write_register(mp->port, CMD_WORD_ADDR, init_ctl_word);

	if (mret < 0) {
		ret = errno;
		err("Error writing drive command word", lp, mip, ret);
	}
#endif

	return ret;
}

int ctl_word_write_mask (mbdport *mp, mbdinfo *mip, logging *lp, uint16_t and,
                         uint16_t or)
{
	int ret;
	int status;

	pthread_mutex_lock(&mp->lock);
	status = modbus_mask_write_register(mp->port, CMD_WORD_ADDR, and, or);
	pthread_mutex_unlock(&mp->lock);

	if (status < 0) {
		err("Error writing modbus register", lp, mip, errno);
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

int perform_action (mbdport *mp, mbdinfo *mip, logging *lp, char *cmd)
{
	int ret;

	if (strcmp(cmd, START_TOKEN) == 0) {
		ret = ctl_word_write_mask(mp, mip, lp, ~RUN_OFF1_MASK,
		                          RUN_OFF1_MASK);
	} else if (strcmp(cmd, STOP_TOKEN) == 0) {
		ret = ctl_word_write_mask(mp, mip, lp, ~RUN_OFF1_MASK,
		                          ~RUN_OFF1_MASK);
	} else {
		logger("Unrecognised command written to " CONTROL_FIFO_PATH,
		       lp, mip);
		ret = -1;
	}

	return ret;
}

void send_ctrl_msg (mbdport *mp, mbdinfo *mip, logging *lp)
{
	size_t n;
	char buf[80];
	int fd;

	fd = open(CONTROL_FIFO_PATH, O_RDONLY);
	n = read(fd, &buf, sizeof(buf) - 1);
	close(fd);

	buf[n] = '\0';

#ifdef NOMODBUS
	printf("%s: got '%s' from %s\n", __func__, buf, CONTROL_FIFO_PATH);
#else
	perform_action(mp, mip, lp, buf);
#endif
}

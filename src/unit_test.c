#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <modbus.h>
#include <regex.h>
#include <time.h>
#include "shared.h"
#include "confparse.h"
#include "time.h"
#include "control.h"

#define NUM_TESTS             5
#define NUM_ITER              5
#define SENSOR_LOGFILE_REGEX  "[0-9]\\+-[0-9a-fA-F]\\{8\\}-[0-9a-fA-F]\\{4\\}-[0-9a-fA-F]\\{4\\}-[0-9a-fA-F]\\{4\\}-[0-9a-fA-F]\\{12\\}\\.log"

#define TEST_UUID_1           "42c4d857-2666-4805-bee8-ed340527e76b"

int match_regex (regex_t *regex, const char *pattern, char *match)
{
	int ret;

	ret = regcomp(regex, pattern, 0);
	if (ret > 0) {
		printf("Error: could not compile regex %s\n", pattern);
		exit(1);
	}

	return regexec(regex, match, 0, NULL, 0);
}

int verify_convert_assign_ul (void)
{
	const unsigned long min = 0;
	int ret = 0;
	int i;
	int count;
	char inputs[NUM_ITER][80] = {"", "129678", "209897892", "28720", "0"};
	unsigned long expected[NUM_ITER] = {ULONG_MAX, 129678, 209897892, 28720, 0};

	count = snprintf(inputs[0], sizeof(inputs[0]), "%lu", ULONG_MAX);

	if ((int) sizeof(inputs[0]) <= count)
		inputs[0][(int) sizeof(inputs[0]) - 1] = '\0';

	for (i = 0; i < NUM_ITER; i++) {
		unsigned long result;

		convert_assign_ul(&result, inputs[i], "UNIT_TEST", min);

		if (result != expected[i]) {
			printf("%s iteration %d : conversion of string \"%s\""
			       "to unsigned long failed: got %lu\n", __func__,
			       i + 1, inputs[i], result);
			ret++;
		} else {
			printf("%s iteration %d passed\n", __func__, i + 1);
		}
	}

	return ret;
}

int verify_convert_assign_uint (void)
{
	const unsigned int min = 0;
	int ret = 0;
	int i;
	int count;
	char inputs[NUM_ITER][80] = {"", "129678", "209897892", "28720", "0"};
	unsigned int expected[NUM_ITER] = {UINT_MAX, 129678, 209897892, 28720, 0};

	count = snprintf(inputs[0], sizeof(inputs[0]), "%u", UINT_MAX);

	if ((int) sizeof(inputs[0]) <= count)
		inputs[0][(int) sizeof(inputs[0]) - 1] = '\0';

	for (i = 0; i < NUM_ITER; i++) {
		unsigned int result;

		convert_assign_uint(&result, inputs[i], "UNIT_TEST", min);

		if (result != expected[i]) {
			printf("%s iteration %d : conversion of string \"%s\""
			       "to unsigned int failed: got %u\n", __func__,
			       i + 1, inputs[i], result);
			ret++;
		} else {
			printf("%s iteration %d passed\n", __func__, i + 1);
		}
	}

	return ret;
}

int verify_convert_assign_retries (void)
{
	int ret = 0;
	int i;
	int count;
	char inputs[NUM_ITER][80] = {"", "129678", "986687", "2469866",
                                     "infinity"};

	int expected[NUM_ITER] = {INT_MAX, 129678, 986687, 2469866, -1};

	count = snprintf(inputs[0], sizeof(inputs[0]), "%d", INT_MAX);

	if ((int) sizeof(inputs[0]) <= count)
		inputs[0][(int) sizeof(inputs[0]) - 1] = '\0';

	for (i = 0; i < NUM_ITER; i++) {
		int result;

		convert_assign_retries(&result, inputs[i]);

		if (result == expected[i] || (strcmp(inputs[i], "infinity") == 0
			&& result == -1)) {
			printf("%s iteration %d passed\n", __func__, i + 1);
		} else {
			printf("%s iteration %d : conversion of string \"%s\""
			       "to int failed: got %d\n", __func__,
			       i + 1, inputs[i], result);
			ret++;
		}
	}

	return ret;
}

int verify_gen_filename (void)
{
	char *gen;
	int ret;
	regex_t rgx;

	gen = gen_filename(TEST_UUID_1);
	ret = match_regex(&rgx, SENSOR_LOGFILE_REGEX, gen);

	if (ret == 0)
		printf("%s passed\n", __func__);
	else if (ret == REG_NOMATCH)
		printf("%s error: generated filename \"%s\" does not match the "
		       "expected pattern\n", __func__, gen);
	else
		printf("%s error while trying to match regex\n", __func__);

	return ret;
}

int verify_ms_to_itimerspec (void)
{
	int i;
	int ret = 0;
	struct itimerspec ts;
	unsigned long msecs[NUM_ITER] = {1000, 100000, 900, 12343, 985741};
	long int expected[NUM_ITER][2] =
	{
		{1, 0},
		{100, 0},
		{0, 900000000},
		{12, 343000000},
		{985, 741000000}
	};

	for (i = 0; i < NUM_ITER; i++) {
		ms_to_itimerspec(&ts, msecs[i]);

		if (ts.it_value.tv_sec == expected[i][0] &&
		    ts.it_interval.tv_sec == expected[i][0] &&
		    ts.it_value.tv_nsec == expected[i][1] &&
		    ts.it_interval.tv_nsec == expected[i][1]) {
			printf("%s iteration %d passed\n", __func__, i + 1);
		} else {
			printf("%s iteration %d millisecond conversion failed\n",
			       __func__, i + 1);
			ret++;
		}
	}

	return ret;
}

void test_usage(char *arg0)
{
	printf("Usage: %s [-n | <num>]\n\n", arg0);
	printf("-n             print out the number of tests available\n");
	printf("<num>          run test number <num>\n");
	printf("no arguments   run all tests\n");
	exit(0);
}

int main (int argc, char *argv[])
{
	int i;
	int testnum;
	int ret = 0;
	int (*tests[NUM_TESTS]) (void) =
		{verify_convert_assign_uint, verify_convert_assign_ul,
		 verify_convert_assign_retries, verify_gen_filename,
	         verify_ms_to_itimerspec};

	if (argc > 1) {
		if (strcmp(argv[1], "-n") == 0) {
			printf("%d\n", NUM_TESTS);
			exit(0);
		} else {
			testnum = atoi(argv[1]);

			if (testnum < 1)
				test_usage(argv[0]);

			ret = tests[testnum - 1]();
		}
	}

	else {
		for (i = 0; i < NUM_TESTS; i++) {
			ret += tests[i](); 
		}
	}

	return ret;
}

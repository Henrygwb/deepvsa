#include <time.h>

void init_alias_time();

void calc_alias_time();

void print_total_alias_time();

void init_runtime();

struct timespec add(struct timespec time1, struct timespec time2);

struct timespec diff(struct timespec start, struct timespec end);

void calc_runtime();

#define LOG_ALIAS_TIME() \
	init_alias_time();

#define RECORD_ALIAS_TIME() \
	calc_alias_time();

#define LOG_RUNTIME(message) \
	calc_runtime(); \
	fprintf(stderr, "%s\n", message);

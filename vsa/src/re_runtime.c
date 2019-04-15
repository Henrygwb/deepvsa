#include "re_runtime.h"
#include "reverse_log.h"


struct timespec total_alias_time = {0};
struct timespec start_alias_time;

void init_alias_time(){
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_alias_time);
}

void calc_alias_time(){
	struct timespec now, diff_time;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
	
	diff_time = diff(start_alias_time, now);
	//LOG(stderr, "Runtime is %lds : %ldms\n", diff_time.tv_sec, 
	//	diff_time.tv_nsec/1000000);
	total_alias_time = add(total_alias_time, diff_time);
	//print_total_alias_time();
}

void print_total_alias_time() {
	LOG(stderr, "Total Alias Time is %lds : %ldms\n", total_alias_time.tv_sec, total_alias_time.tv_nsec/1000000);
}

struct timespec start_time;

void init_runtime(){
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
	//LOG(stderr, "[%lds : %ldns] Initializing the runtime\n", 
	//	start_time.tv_sec, start_time.tv_nsec);
}

struct timespec add(struct timespec time1, struct timespec time2)
{
	struct timespec temp;
	if ((time1.tv_nsec+time2.tv_nsec) < 1000000000) {
		temp.tv_sec  = time1.tv_sec  + time2.tv_sec;
		temp.tv_nsec = time1.tv_nsec + time2.tv_nsec;
	} else {
		temp.tv_sec  = time1.tv_sec  + time2.tv_sec+1;
		temp.tv_nsec = time1.tv_nsec + time2.tv_nsec-1000000000;
	}
	return temp;
}

struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}

void calc_runtime(){
	struct timespec now;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
	
	LOG(stderr, "Runtime is %lds : %ldms\n", diff(start_time, now).tv_sec, 
		diff(start_time, now).tv_nsec/1000000);
}

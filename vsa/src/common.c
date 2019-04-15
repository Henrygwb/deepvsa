#include <stdio.h>
#include <string.h>
#include "reverse_log.h"

unsigned long count_linenum_ptlog(char *filename){
	char line[256];
	FILE *file;
	if ((file = fopen(filename, "r")) == NULL) {
		LOG(stderr, "ERROR: open error\n");
		return -1;
	}
	unsigned long linenum = 0;
	while (fgets(line, sizeof line, file) != NULL) {
		if ((strncmp(line, "[disabled]", 10) == 0)) continue;
		if ((strncmp(line, "[enabled]", 9) == 0)) continue;
		if ((strncmp(line, "[resumed]", 9) == 0)) continue;
		linenum++;
	}

	LOG(stdout, "RESULT: Valid Address Number - 0x%lx\n", linenum);
	return linenum;
}

unsigned long gcd(unsigned long a, unsigned long b) {
	int tmp;
	while(b != 0) {
		tmp = b;
		b = a % b;
		a = tmp;
	}
	return a;
}


//count the number of instructions that have valid data logging
unsigned long count_linenum(char * filename){
	char line[256];
	FILE *file;
	unsigned long linenum;

	if ((file = fopen(filename, "r")) == NULL) {
		LOG(stderr, "ERROR: cannot open file containing data log\n");
		return -1;
	}

	linenum = 0;
	while(fgets(line, sizeof(line), file) != NULL){
		linenum++;
	}

	LOG(stdout, "RESULT: Valid Address Number - 0x%lx\n", linenum);
	return linenum;
}

#ifndef __GLOBAL__
#define __GLOBAL__

#include <libdis.h>
#include "elf_core.h"
#include "elf_binary.h"

typedef struct input_struct {
	char *case_path;
	char *core_path;	// coredump
	char *inst_path; 	// instruction address
	char *libs_path;   	// library 
	char *log_path;	// register status
	char *xmm_path;		// xmm registers
	// DL input data
	char *bin_path;   	// binary representation
	//char *memop_path;	// memory operands
	char *region_path;   	// region for memory operands
	// DL input data
#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	// VSA input data
	char *heu_path;		// vsa heuristic
#endif
} input_st;

extern input_st input_data;

void set_input_data(char *path);
void clean_input_data(input_st input_data);

char * get_core_path(void);
char * get_inst_path(void);
char * get_libs_path(void);
char * get_log_path(void);
char * get_xmm_path(void);

char * get_bin_path(void);
//char * get_memop_path(void);
char * get_region_path(void);

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
char * get_heu_path(void);
#endif

unsigned long count_linenum(char *filename);
unsigned long count_linenum_ptlog(char *filename);
unsigned long gcd(unsigned long a, unsigned long b);
#endif

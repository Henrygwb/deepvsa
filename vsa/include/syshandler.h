#ifndef __SYSHANDLER__
#define __SYSHANDLER__

#include "reverse_exe.h"

typedef struct sys_index_pair{
	unsigned int sys_id;
	int index; 
}sys_index_pair_t;

extern sys_index_pair_t sys_index_tab[];

extern const int nsys;

typedef void (*sys_resolver_func)(re_list_t * instnode);

extern sys_resolver_func syscall_resolver[]; 

// system call resolvers
void sys_socketcall_resolver(re_list_t *instnode);

void sys_write_resolver(re_list_t *instnode);

void sys_close_resolver(re_list_t *instnode);

void sys_fcntl64_resolver(re_list_t *instnode);

void sys_nanosleep_resolver(re_list_t *instnode);

void sys_mmap2_resolver(re_list_t *instnode);

void sys_dup_resolver(re_list_t *instnode);

void sys_mremap_resolver(re_list_t *instnode);

void sys_munmap_resolver(re_list_t *instnode);

void sys_lseek_resolver(re_list_t *instnode);

void sys__llseek_resolver(re_list_t *instnode);

void sys_open_resolver(re_list_t *instnode);

void sys_brk_resolver(re_list_t *instnode);

void sys_stat64_resolver(re_list_t *instnode);

void sys_fstat64_resolver(re_list_t *instnode);

void sys_read_resolver(re_list_t *instnode);

void sys_getrusage_resolver(re_list_t *instnode);

void sys_rt_sigaction_resolver(re_list_t *instnode);

void sys_time_resolver(re_list_t *instnode);
#endif

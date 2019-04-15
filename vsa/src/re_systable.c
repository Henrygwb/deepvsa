#include <sys/syscall.h>
#include "syshandler.h"


// function points table for system call resolver
// Be careful! If you uncomment one instruction type in the middle,
// please modify all the following entries
sys_index_pair_t sys_index_tab[] = {
	{SYS_socketcall, 0x0},
	{SYS_write, 0x1},
	{SYS_close, 0x2},
	{SYS_fcntl64, 0x3},
	{SYS_nanosleep, 0x4},
	{SYS_mmap2, 0x5},
	{SYS_lseek, 0x6},
	{SYS__llseek, 0x7},
	{SYS_open, 0x8},
	{SYS_brk, 0x9},
	{SYS_stat64, 0xA},
	{SYS_fstat64, 0xB},
	{SYS_read, 0xC},
	{SYS_mremap, 0xD},
	{SYS_munmap, 0xE},
	{SYS_dup, 0xF},
	{SYS_getrusage, 0x10},
	{SYS_rt_sigaction, 0x11},
	{SYS_time, 0x12}
};

const int nsys = sizeof(sys_index_tab) / sizeof (sys_index_pair_t);

sys_resolver_func syscall_resolver[] = {
	&sys_socketcall_resolver,	// 0x0
	&sys_write_resolver,		// 0x1
	&sys_close_resolver,		// 0x2
	&sys_fcntl64_resolver,		// 0x3
	&sys_nanosleep_resolver,		// 0x4
	&sys_mmap2_resolver,		// 0x5
	&sys_lseek_resolver,		// 0x6
	&sys__llseek_resolver,		// 0x7
	&sys_open_resolver,		// 0x8
	&sys_brk_resolver,		// 0x9
	&sys_stat64_resolver,		// 0xA
	&sys_fstat64_resolver,		// 0xB
	&sys_read_resolver,		// 0xC
	&sys_mremap_resolver,		// 0xD
	&sys_munmap_resolver,		// 0xE
	&sys_dup_resolver,		// 0xF
	&sys_getrusage_resolver,	// 0x10
	&sys_rt_sigaction_resolver,	// 0x11
	&sys_time_resolver		// 0x12
};

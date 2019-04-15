#include <sys/syscall.h>
#include <linux/net.h>
#include "syshandler.h"
#include "reverse_log.h"

void sys_socketcall_resolver(re_list_t *instnode) {
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	valset_u rv1;
	int recv_size;
	bool arg1_known;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// Should we fork corelist here and resolve parameters of system call?

	if (!(CAST2_USE(src[0]->node)->val_known) ||
	    !(CAST2_DEF(dst[0]->node)->val_stat & AfterKnown))
		return;

	switch (CAST2_USE(src[0]->node)->val.dword) {
	case SYS_RECV:
		// generate use-define by the semantics of system call
		// if parameters are not obtained, just return
		if (!CAST2_USE(src[1]->node)->val_known) return;
		// get the first parameter of syscall
		arg1_known = get_parameter_of_sysenter(instnode,
			CAST2_USE(src[1]->node)->val.dword + 4,
			op_dword, &rv1);
		if (arg1_known) {
			// rv1 : base address of buffer
			// rv2 : size of buffer
			// remove system call block at first
			remove_from_umemlist(dst[1]);
			list_del(&dst[1]->list);

			recv_size = CAST2_DEF(dst[0]->node)->afterval.dword;

			assert(recv_size > 0);

			// op_byte is obtained from semantics of system call
			extend_corelist_from_sysenter(instnode, rv1.dword, op_byte, recv_size);
		}
		break;
	case SYS_SENDTO:
		break;
	case SYS_SOCKET:
		break;
	case SYS_CONNECT:
		break;
	case SYS_SEND:
		break;
	default:
		LOG(stderr, "ebx id %ld\n", CAST2_USE(src[0]->node)->val.dword);
		assert(0 && "not implemented");
		break;
	}
}

void sys_write_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_close_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_fcntl64_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_nanosleep_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_mmap2_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_mremap_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_munmap_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_dup_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_lseek_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys__llseek_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_open_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_brk_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_stat64_resolver(re_list_t *instnode) {
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);
}

void sys_fstat64_resolver(re_list_t *instnode) {
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

}

void sys_read_resolver(re_list_t *instnode) {
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	int recv_size;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// ecx known and eax after value known
	if (!(CAST2_USE(src[1]->node)->val_known) ||
	    !(CAST2_DEF(dst[0]->node)->val_stat & AfterKnown))
		return;

	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);

	//LOG(stdout, "buffer is %lx\n", CAST2_USE(src[1]->node)->val.dword);
	//LOG(stdout, "received size is %lx\n", CAST2_DEF(dst[0]->node)->afterval.dword);

	//FIXME: we may use the second parameter here when return value is unknown
	recv_size = CAST2_DEF(dst[0]->node)->afterval.dword;

	assert(recv_size > 0);

	// op_byte is obtained from semantics of system call
	extend_corelist_from_sysenter(instnode, CAST2_USE(src[1]->node)->val.dword, op_byte, recv_size);
}

void sys_getrusage_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);

	// it does not affect the user space, remove syscall block
	remove_from_umemlist(dst[1]);
	list_del(&dst[1]->list);
}

void sys_rt_sigaction_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);
}

void sys_time_resolver(re_list_t *instnode) {
	// Not related to user memory space
	re_list_t *src[NOPD], *dst[NOPD];
	int nuse, ndef;
	re_list_t re_deflist, re_uselist;

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);

	traverse_inst_operand(instnode, src, dst, &re_uselist, &re_deflist, &nuse, &ndef);	

	assert(nuse == 5 && ndef == 2);
}

#include "insthandler.h"
#include "syshandler.h"

void sys_handler(re_list_t *instnode){
	x86_insn_t* inst;

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;
	if (strcmp(inst->mnemonic, "sysenter") == 0) {
		sysenter_handler(instnode);
	} else if (strcmp(inst->mnemonic, "rdtsc") == 0) {
		rdtsc_handler(instnode);
	} else {
		assert("Other instruction with type 0xE000" && 0);
	}
}

void halt_handler(re_list_t *instnode){
	assert(0);
}

void in_handler(re_list_t *instnode){
	//assert(0);
}

void out_handler(re_list_t *instnode){
	assert(0);
}

void sysenter_handler(re_list_t *instnode){
	x86_insn_t* inst;
	x86_op_t *eax, *block, *tmpopd, ebx, ecx, edx, esi, edi;
	x86_reg_t reg;
	re_list_t re_deflist, re_uselist, re_instlist;  	

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	print_all_operands(inst);

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);
	INIT_LIST_HEAD(&re_instlist.instlist);	

	eax = x86_implicit_operand_1st(inst);

	add_new_define(eax);

	// add all the parameters here
	x86_reg_from_id(get_ebx_id(), &reg);
	INIT_REGOPD(&ebx, op_register, op_dword, op_read, reg);
	tmpopd = add_new_implicit_operand(inst, &ebx);
	add_new_use(tmpopd, Opd);

	x86_reg_from_id(get_ecx_id(), &reg);
	INIT_REGOPD(&ecx, op_register, op_dword, op_read, reg);
	tmpopd = add_new_implicit_operand(inst, &ecx);
	add_new_use(tmpopd, Opd);

	x86_reg_from_id(get_edx_id(), &reg);
	INIT_REGOPD(&edx, op_register, op_dword, op_read, reg);
	tmpopd = add_new_implicit_operand(inst, &edx);
	add_new_use(tmpopd, Opd);

	x86_reg_from_id(get_esi_id(), &reg);
	INIT_REGOPD(&esi, op_register, op_dword, op_read, reg);
	tmpopd = add_new_implicit_operand(inst, &esi);
	add_new_use(tmpopd, Opd);

	x86_reg_from_id(get_edi_id(), &reg);
	INIT_REGOPD(&edi, op_register, op_dword, op_read, reg);
	tmpopd = add_new_implicit_operand(inst, &edi);
	add_new_use(tmpopd, Opd);

	// insert one memory [0] so that no one could resolve it
	// take it as one block as you don't have information of syscall

	memset(&reg, 0, sizeof(x86_reg_t));

	block = (x86_op_t *)malloc(sizeof(x86_op_t));
	INIT_MEM(block, op_expression, op_dword, op_write, reg, reg);
	add_new_define(block);

	add_to_instlist(instnode, &re_instlist);
	re_resolve(&re_deflist, &re_uselist, &re_instlist);
}

void rdtsc_handler(re_list_t *instnode) {
	x86_insn_t* inst;
	x86_op_t *eax, *edx;
	re_list_t re_deflist, re_uselist, re_instlist;  	

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	print_all_operands(inst);

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);
	INIT_LIST_HEAD(&re_instlist.instlist);	

	edx = x86_implicit_operand_1st(inst);
	eax = x86_implicit_operand_2nd(inst);

	add_new_define(edx);
	add_new_define(eax);

	add_to_instlist(instnode, &re_instlist);
	re_resolve(&re_deflist, &re_uselist, &re_instlist);
}

void cpuid_handler(re_list_t *instnode){
	x86_insn_t* inst;
	x86_op_t *eax, *ebx, *ecx, *edx;
	re_list_t re_deflist, re_uselist, re_instlist;  	

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	print_all_operands(inst);

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);
	INIT_LIST_HEAD(&re_instlist.instlist);	

	eax = x86_implicit_operand_1st(inst);
	ecx = x86_implicit_operand_2nd(inst);
	edx = x86_implicit_operand_3rd(inst);
	ebx = x86_implicit_operand_4th(inst);

	add_new_define(eax);
	add_new_define(ecx);
	add_new_define(edx);
	add_new_define(ebx);

	add_new_use(eax, Opd);

	add_to_instlist(instnode, &re_instlist);
	re_resolve(&re_deflist, &re_uselist, &re_instlist);
}

void sys_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){

	x86_insn_t* instruction;

        instruction = re_ds.instlist + CAST2_INST(inst->node)->inst_index;

	if (strcmp(instruction->mnemonic, "sysenter") == 0) {
		sysenter_resolver(inst, re_deflist, re_uselist);
	} else if (strcmp(instruction->mnemonic, "rdtsc") == 0) {
		rdtsc_resolver(inst, re_deflist, re_uselist);
	} else {
		assert("Other instruction with type 0xE000" && 0);
	}
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void sys_vs_handler(re_list_t *instnode) {
	
	x86_insn_t* inst;

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	if (strcmp(inst->mnemonic, "sysenter") == 0) {
		sysenter_vs_handler(instnode);
	} else if (strcmp(inst->mnemonic, "rdtsc") == 0) {
		rdtsc_vs_handler(instnode);
	} else {
		assert("Other instruction with type 0xE000" && 0);
	}
}
#endif

void halt_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
	assert(0);
}

void in_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
	//assert(0);
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void in_vs_handler(re_list_t *instnode) {
	assert(0);
}
#endif

void out_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
	assert(0);
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void out_vs_handler(re_list_t *instnode) {
	assert(0);
}
#endif

void sysenter_resolver(re_list_t* instnode, re_list_t *re_deflist, re_list_t *re_uselist){
	// due to use-define chain obtained by system
	// we cannot use NOPD constant as length of operands array
	re_list_t **dst;
	re_list_t *src[NOPD];
	size_t num_opd = 0x4000;
	int nuse, ndef;

	dst = (re_list_t **)malloc(num_opd * sizeof(re_list_t *));

	traverse_inst_operand(instnode, src, dst, re_uselist, re_deflist, &nuse, &ndef);	
	//print_info_of_current_inst(instnode);

	assert(nuse == 5 && (ndef > 2 || ndef == 2 || ndef == 1));

	// if you have resolved the system call
	// 1. system call affects the user space memory - ndef > 2
	// 2. system call does not affect the user space memory - ndef == 1
	if (ndef > 2 || ndef == 1) return;

	if (CAST2_DEF(dst[0]->node)->val_stat & BeforeKnown) {
		unsigned long sys_id = CAST2_DEF(dst[0]->node)->beforeval.dword;
		int sys_index = syscall_to_index(sys_id);
		//LOG(stdout, "System call id %ld\n", sys_id);

		if (sys_index != -1) {
			syscall_resolver[sys_index](instnode);
		} else {
			LOG(stderr, "SysCall ID is %ld\n", sys_id);
			assert(0);
		}

		traverse_inst_operand(instnode, src, dst, re_uselist, re_deflist, &nuse, &ndef);	

		assert(nuse == 5 && (ndef > 2 || ndef == 2 || ndef == 1));
	}
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void sysenter_vs_handler(re_list_t *instnode){
	// do nothing, leave tasks to RE
}
#endif

void rdtsc_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
	
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void rdtsc_vs_handler(re_list_t *instnode){
	
	re_list_t *entry;
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;
	re_si si;
	
	// traverse all the operands to fill all value set
	// (including value set and address value) of operands
	vs_traverse_inst_operand(instnode, src, dst, &nuse, &ndef);
	assert(nuse == 0 && ndef == 2);

	si_set_full(&si);
	add_new_valset(&CAST2_DEF(dst[0]->node)->aft_valset,
			CONST_REGION, 0, si);
	add_new_valset(&CAST2_DEF(dst[1]->node)->aft_valset,
			CONST_REGION, 0, si);
}
#endif

void cpuid_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
	LOG(stdout, "CPUID resolver\n");
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void cpuid_vs_handler(re_list_t *instnode) {
	LOG(stdout, "CPUID value set handler\n");
}
#endif

#include "insthandler.h"

void int_handler(re_list_t *instnode){
	x86_insn_t* inst;
	x86_op_t eax, *tmpopd;
	x86_reg_t reg;
	re_list_t re_deflist, re_uselist, re_instlist;

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	print_all_operands(inst);

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);
	INIT_LIST_HEAD(&re_instlist.instlist);

	x86_reg_from_id(get_eax_id(), &reg);
	INIT_REGOPD(&eax, op_register, op_dword, op_write, reg);
	tmpopd = add_new_implicit_operand(inst, &eax);
	add_new_define(tmpopd);

	add_to_instlist(instnode, &re_instlist);
	re_resolve(&re_deflist, &re_uselist, &re_instlist);
}


void int_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
}

void int_vs_handler(re_list_t *instnode){
}


#include "insthandler.h"

static bool is_jcc_satisfied(re_list_t *instnode) {
	re_list_t *jcc_inst;
	int inst_index;
	uint32_t target;
	x86_op_t *opd;

	jcc_inst = find_first_jcc(instnode);
	assert(jcc_inst);
	inst_index = CAST2_INST(jcc_inst->node)->inst_index;
	opd = x86_operand_1st(re_ds.instlist + inst_index);
	//LOG(stdout, "operand type %d\n", opd->type);
	if (opd->type == op_relative_near) {
		//LOG(stdout, "relative_near = 0x%x, addr = 0x%x\n",
		//	    opd->data.relative_near,
		//	    re_ds.instlist[inst_index].addr);
		target = opd->data.relative_near + re_ds.instlist[inst_index].addr
			+ re_ds.instlist[inst_index].size;
	} else if (opd->type == op_relative_far) {
		//LOG(stdout, "relative_far = 0x%x, addr = 0x%x\n",
		//	    opd->data.relative_far,
		//	    re_ds.instlist[inst_index].addr);
		target = opd->data.relative_far + re_ds.instlist[inst_index].addr
			+ re_ds.instlist[inst_index].size;
	} else {
		assert(0 && "Other Operand Type");
	}	
	//LOG(stdout, "target = 0x%x, addr = 0x%x\n", target,
	//	      re_ds.instlist[inst_index-1].addr);

	return (target == re_ds.instlist[inst_index-1].addr);
}

// through jcc and control flow to verify result of cmp instruction
static void verify_relation_for_cmp(re_list_t *instnode) {

	LOG(stdout, "trying to verify relation for cmp instruction\n");
	// jg/jz/jc/jl target and address of next instruction
	
	re_list_t *jcc_inst;
	int inst_index;
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;

	jcc_inst = find_first_jcc(instnode);
	assert(jcc_inst);
	//print_info_of_current_inst(jcc_inst);
	inst_index = CAST2_INST(jcc_inst->node)->inst_index;

	obtain_inst_operand(instnode, src, dst, &nuse, &ndef);
	assert(nuse == 2);

	switch (get_jcc_type(re_ds.instlist + inst_index)) {
		case JZ:
			if (is_jcc_satisfied(instnode)) {
				// test eax, eax
				// set eax 0
				LOG(stdout, "jz condition is satisfied\n");
			} else {
				LOG(stdout, "jz condition is not satisfied\n");
				// how to interpret eax != 0
			}
			break;
		case JNZ:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jnz condition is satisfied\n");
			} else {
				LOG(stdout, "jnz condition is not satisfied\n");
				// test eax, eax
				// set eax 0
			}
			break;
		case JA:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "ja condition is satisfied\n");
			} else {
				LOG(stdout, "ja condition is not satisfied\n");
			}
			break;
		case JB:
		case JC:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jb/jc condition is satisfied\n");
			} else {
				LOG(stdout, "jb/jc condition is not satisfied\n");
			}
			break;
		case JBE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jbe condition is satisfied\n");
			} else {
				LOG(stdout, "jbe condition is not satisfied\n");
			}
			break;
		case JL:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jl condition is satisfied\n");
			} else {
				LOG(stdout, "jl condition is not satisfied\n");
			}
			break;
		case JECXZ:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jecxz condition is satisfied\n");
			} else {
				LOG(stdout, "jecxz condition is not satisfied\n");
			}
			break;
		case JLE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jle condition is satisfied\n");
			} else {
				LOG(stdout, "jle condition is not satisfied\n");
			}
			break;
		case JNB:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jnb condition is satisfied\n");
			} else {
				LOG(stdout, "jnb condition is not satisfied\n");
			}
			break;
		case JNBE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jnbe condition is satisfied\n");
			} else {
				LOG(stdout, "jnbe condition is not satisfied\n");
			}
			break;
		case JG:
		case JNLE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jg/jnle condition is satisfied\n");
			} else {
				LOG(stdout, "jg/jnle condition is not satisfied\n");
			}
			break;
		case JGE:
		case JNL:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jnl condition is satisfied\n");
			} else {
				LOG(stdout, "jnl condition is not satisfied\n");
			}
			break;
		case JNC:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jnc condition is satisfied\n");
			} else {
				LOG(stdout, "jnc condition is not satisfied\n");
			}
			break;
		case JNS:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jns condition is satisfied\n");
			} else {
				LOG(stdout, "jns condition is not satisfied\n");
			}
			break;
		case JP:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jp condition is satisfied\n");
			} else {
				LOG(stdout, "jp condition is not satisfied\n");
			}
			break;
		case JPE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jpe condition is satisfied\n");
			} else {
				LOG(stdout, "jpe condition is not satisfied\n");
			}
			break;
		case JS:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "js condition is satisfied\n");
			} else {
				LOG(stdout, "js condition is not satisfied\n");
			}
			break;
		default:
			assert(0);
	}
}

// through jcc and control flow to verify result of test instruction
static void verify_relation_for_test(re_list_t *instnode) {

	LOG(stdout, "trying to verify relation for test instruction\n");
	// jz target and address of next instruction
	
	re_list_t *jcc_inst;
	int inst_index;
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;

	jcc_inst = find_first_jcc(instnode);
	assert(jcc_inst);
	//print_info_of_current_inst(jcc_inst);
	inst_index = CAST2_INST(jcc_inst->node)->inst_index;

	obtain_inst_operand(instnode, src, dst, &nuse, &ndef);
	assert(nuse == 2);

	switch (get_jcc_type(re_ds.instlist + inst_index)) {
		case JBE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jbe condition is satisfied\n");
			} else {
				LOG(stdout, "jbe condition is not satisfied\n");
			}
			break;
		case JZ:
			if (is_jcc_satisfied(instnode)) {
				// test eax, eax
				// set eax 0
				LOG(stdout, "jz condition is satisfied\n");
			} else {
				LOG(stdout, "jz condition is not satisfied\n");
				// how to interpret eax != 0
			}
			break;
		case JNZ:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jnz condition is satisfied\n");
			} else {
				LOG(stdout, "jnz condition is not satisfied\n");
				// test eax, eax
				// set eax 0
			}
			break;
		case JS:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "js condition is satisfied\n");
			} else {
				LOG(stdout, "js condition is not satisfied\n");
			}
			break;
		case JNS:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jns condition is satisfied\n");
			} else {
				LOG(stdout, "jns condition is not satisfied\n");
			}
			break;
		case JG:
		case JNLE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jg/jnle condition is satisfied\n");
			} else {
				LOG(stdout, "jg/jnle condition is not satisfied\n");
			}
			break;
		case JLE:
			if (is_jcc_satisfied(instnode)) {
				LOG(stdout, "jle condition is satisfied\n");
			} else {
				LOG(stdout, "jle condition is not satisfied\n");
			}
			break;
		default:
			assert(0);
	}

}

void test_handler(re_list_t *instnode){

	x86_insn_t* inst;
	x86_op_t *src1, *src2;
	re_list_t re_deflist, re_uselist, re_instlist;  	
	re_list_t *usesrc1, *usesrc2;

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	src1 = x86_get_dest_operand(inst);
	src2 = x86_get_src_operand(inst);

	convert_offset_to_exp(src1);
	convert_offset_to_exp(src2);	

	//	for debugginf use	
	print_all_operands(inst);

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);
	INIT_LIST_HEAD(&re_instlist.instlist);	

	switch(get_operand_combine(inst)){
		case dest_register_src_register:
			usesrc1 = add_new_use(src1, Opd);
			usesrc2 = add_new_use(src2, Opd);
			break;
		case dest_register_src_imm:
			usesrc1 = add_new_use(src1, Opd);
			usesrc2 = add_new_use(src2, Opd);
			break;
		case dest_register_src_expression:
			usesrc1 = add_new_use(src1, Opd);	
			usesrc2 = add_new_use(src2, Opd);		
			split_expression_to_use(src2);
			break;
		case dest_expression_src_register:
			usesrc1 = add_new_use(src1, Opd);	
			split_expression_to_use(src1);
			usesrc2 = add_new_use(src2, Opd);		
			break;
		case dest_expression_src_imm:
			usesrc1 = add_new_use(src1, Opd);	
			split_expression_to_use(src1);
			usesrc2 = add_new_use(src2, Opd);		
			break;
		default:
			assert(0);
	}

	//list_add(&instnode->instlist, &re_instlist.instlist);
	add_to_instlist(instnode, &re_instlist);

	re_resolve(&re_deflist, &re_uselist, &re_instlist);
}


void test_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){

	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;

	traverse_inst_operand(inst,src,dst,re_uselist, re_deflist, &nuse, &ndef);	

	assert(nuse == 2 && ndef == 0);
}


#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void test_vs_handler(re_list_t *instnode){

	//print_info_of_current_inst(instnode);

	re_list_t *entry;
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;
	
	// traverse all the operands to fill all value set
	// (including value set and address value) of operands
	vs_traverse_inst_operand(instnode, src, dst, &nuse, &ndef);
	assert(nuse == 2);
	
	// post heuristic with control flow
	//verify_relation_for_test(instnode);
}
#endif


void cmp_handler(re_list_t *instnode){

	x86_insn_t* inst;
	x86_op_t *src1, *src2;
	re_list_t re_deflist, re_uselist, re_instlist;  	
	re_list_t *usesrc1, *usesrc2;

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	src1 = x86_get_dest_operand(inst);
	src2 = x86_get_src_operand(inst);

	convert_offset_to_exp(src1);
	convert_offset_to_exp(src2);	

	//	for debugginf use	
	print_all_operands(inst);

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);
	INIT_LIST_HEAD(&re_instlist.instlist);	

	switch(get_operand_combine(inst)){
		case dest_register_src_register:
			usesrc1 = add_new_use(src1, Opd);
			usesrc2 = add_new_use(src2, Opd);
			break;
		case dest_register_src_imm:
			usesrc1 = add_new_use(src1, Opd);
			usesrc2 = add_new_use(src2, Opd);
			break;
		case dest_register_src_expression:
			usesrc1 = add_new_use(src1, Opd);	
			usesrc2 = add_new_use(src2, Opd);		
			split_expression_to_use(src2);
			break;
		case dest_expression_src_register:
			usesrc1 = add_new_use(src1, Opd);	
			split_expression_to_use(src1);
			usesrc2 = add_new_use(src2, Opd);		
			break;
		case dest_expression_src_imm:
			usesrc1 = add_new_use(src1, Opd);	
			split_expression_to_use(src1);
			usesrc2 = add_new_use(src2, Opd);		
			break;
		default:
			assert(0);
	}

	//list_add(&instnode->instlist, &re_instlist.instlist);
	add_to_instlist(instnode, &re_instlist);

	re_resolve(&re_deflist, &re_uselist, &re_instlist);
}


void cmp_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;

	traverse_inst_operand(inst,src,dst,re_uselist, re_deflist, &nuse, &ndef);	

	assert(nuse == 2 && ndef == 0);
}


#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void cmp_vs_handler(re_list_t *instnode){

	//print_info_of_current_inst(instnode);

	re_list_t *entry;
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;
	
	// traverse all the operands to fill all value set
	// (including value set and address value) of operands
	vs_traverse_inst_operand(instnode, src, dst, &nuse, &ndef);
	assert(nuse == 2);

	// post heuristic with control flow
	//verify_relation_for_cmp(instnode);
}
#endif

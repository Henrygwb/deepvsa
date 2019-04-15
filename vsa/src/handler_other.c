#include "insthandler.h"

#define PXOR 0x00
#define MOVDQU 0x01
#define PMOVMSKB 0x02
#define PCMPEQB 0x03
#define PMINUB 0x04
#define MOVAPS 0x05
#define MOVDQA 0x06
#define MOVQ 0x07
#define MOVD 0x08
#define PSHUFD 0x09
#define PUNPCKLBW 0x0A
#define PTEST 0x0B
#define PSUBB 0x0C
#define PALIGNR 0x0D
#define PMAXUB 0x0E
#define POR 0x0F
#define PAND 0x10
#define PCMPGTB 0x11

#define BADINST -1

typedef struct { char *key; int val; } string_kv;
static string_kv insttable[] = {
    {"pxor", PXOR},
    {"movdqu", MOVDQU},
    {"pmovmskb", PMOVMSKB},
    {"pcmpeqb", PCMPEQB},
    {"pminub", PMINUB},
    {"movaps", MOVAPS},
    {"movdqa", MOVDQA},
    {"movq", MOVQ},
    {"movd", MOVD},
    {"pshufd", PSHUFD},
    {"punpcklbw", PUNPCKLBW},
    {"ptest", PTEST},
    {"psubb", PSUBB},
    {"palignr", PALIGNR},
    {"pmaxub", PMAXUB},
    {"pand", PAND},
    {"por", POR},
    {"pcmpgtb", PCMPGTB}
};

#define NKEYS (sizeof(insttable)/sizeof(string_kv))

int string2int(char *key)
{
    int i;
    for (i=0; i < NKEYS; i++) {
	string_kv sym = insttable[i];	
        if (strcmp(sym.key, key) == 0)
            return sym.val;
    }
    return BADINST;
}

void unknown_handler(re_list_t * instnode){
	//Handling all unknown instructions here
	//Just for now. Need to improve the instruction classification in the future. 
	//really need to do so many if else?
	
	x86_insn_t* inst;
	inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;


	switch(string2int(inst->mnemonic)){

		case PXOR:
			pxor_handler(instnode);
			break;

		case POR:
			por_handler(instnode);
			break;

		case PAND:
			pand_handler(instnode);
			break;

		case MOVDQU:
			movdqu_handler(instnode);
			break;


		case PMOVMSKB:
			pmovmskb_hanlder(instnode);
			break;

		case PCMPEQB:
			pcmpeqb_handler(instnode);
			break;

		case PMINUB: 
			pminub_handler(instnode);
			break;

		case MOVAPS: 
			movaps_handler(instnode);
			break;

		case MOVDQA:
			movdqa_handler(instnode);
			break;
				
		case MOVQ: 
			movq_handler(instnode);
			break;

		case MOVD:
			movq_handler(instnode);
			break;

		case PSHUFD:
			pshufd_handler(instnode);
			break;

		case PUNPCKLBW:
			punpcklbw_handler(instnode);
			break;

		case PTEST:
			ptest_handler(instnode);
			break;

		case PSUBB:
			psubb_handler(instnode);
			break;

		case PALIGNR:
			palignr_handler(instnode);
			break;

		case PMAXUB:
			pmaxub_handler(instnode);
			break;

		case PCMPGTB:
			pcmpgtb_handler(instnode);	
			break;
/*
		case BADINST:
			LOG(stdout, "Warning: bad instruction\n");
			break;
*/
		default:
			assert(0);
			LOG(stdout, "Error: this will never happen\n");
			break;
		}
}

void unknown_resolver(re_list_t* instnode, re_list_t *re_deflist, re_list_t *re_uselist){


	x86_insn_t* inst;
	inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	switch(string2int(inst->mnemonic)){

		case PXOR:
			pxor_resolver(instnode, re_deflist, re_uselist);
			break;

		case POR:
			por_resolver(instnode, re_deflist, re_uselist);
			break;

		case PAND:
			pand_resolver(instnode, re_deflist, re_uselist);
			break;

		case MOVDQU: 
			movdqu_resolver(instnode, re_deflist, re_uselist);	
			break;	

		case PMOVMSKB:
			pmovmskb_resolver(instnode, re_deflist, re_uselist);			
			break;

		case PCMPEQB:
			pcmpeqb_resolver(instnode, re_deflist, re_uselist);
			break; 
		
		case PMINUB:
			pminub_resolver(instnode, re_deflist, re_uselist);
			break;

		case MOVAPS:
			movaps_resolver(instnode, re_deflist, re_uselist);
			break;

		case MOVDQA:
			movdqa_resolver(instnode, re_deflist, re_uselist);
			break;

		case MOVQ: 
			movq_resolver(instnode, re_deflist, re_uselist);
			break;

		case MOVD:
			movq_resolver(instnode, re_deflist, re_uselist);
			break;

		case PSHUFD:
			pshufd_resolver(instnode, re_deflist, re_uselist);
			break;

		case PUNPCKLBW:
			punpcklbw_resolver(instnode, re_deflist, re_uselist);
			break;

		case PTEST:
			ptest_resolver(instnode, re_deflist, re_uselist);
			break;

		case PSUBB:
			psubb_resolver(instnode, re_deflist, re_uselist);
			break;

		case PALIGNR:
			palignr_resolver(instnode, re_deflist, re_uselist);
			break;

		case PMAXUB:
			pmaxub_resolver(instnode, re_deflist, re_uselist);
			break;

		case PCMPGTB:
			pcmpgtb_resolver(instnode, re_deflist, re_uselist);
			break;

		default:
			assert(0);
			break;	
	}
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void unknown_vs_handler(re_list_t * instnode){

	x86_insn_t* inst;
	inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

	switch(string2int(inst->mnemonic)){

		case PXOR:
			pxor_vs_handler(instnode);
			break;

		case POR:
			por_vs_handler(instnode);
			break;

		case PAND:
			pand_vs_handler(instnode);
			break;

		case MOVDQU:
			movdqu_vs_handler(instnode);
			break;

		case PMOVMSKB:
			pmovmskb_vs_handler(instnode);
			break;

		case PCMPEQB:
			pcmpeqb_vs_handler(instnode);
			break;

		case PMINUB: 
			pminub_vs_handler(instnode);
			break;

		case MOVAPS: 
			movaps_vs_handler(instnode);
			break;

		case MOVDQA:
			movdqa_vs_handler(instnode);
			break;
				
		case MOVQ: 
			movq_vs_handler(instnode);
			break;

		case MOVD:
			movq_vs_handler(instnode);
			break;

		case PSHUFD:
			pshufd_vs_handler(instnode);
			break;

		case PUNPCKLBW:
			punpcklbw_vs_handler(instnode);
			break;

		case PTEST:
			ptest_vs_handler(instnode);
			break;

		case PALIGNR:
			palignr_vs_handler(instnode);
			break;

		case PSUBB:
			psubb_vs_handler(instnode);
			break;

		case PMAXUB:
			pmaxub_vs_handler(instnode);
			break;

		case PCMPGTB:
			pcmpgtb_vs_handler(instnode);
			break;

		default:
			assert(0);
			break;	
	}
}
#endif

void nop_handler(re_list_t * instnode){
}

void nop_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void nop_vs_handler(re_list_t * instnode){
}
#endif

void szconv_handler(re_list_t * instnode){

	x86_insn_t* inst;
	x86_op_t *src, *dst;
	re_list_t re_deflist, re_uselist, re_instlist;  	
	re_list_t *def, *usesrc;

        inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;
	src = x86_implicit_operand_1st(inst);
	dst = x86_implicit_operand_2nd(inst);

	//	for debugginf use	
	print_all_operands(inst);

	INIT_LIST_HEAD(&re_deflist.deflist);
	INIT_LIST_HEAD(&re_uselist.uselist);
	INIT_LIST_HEAD(&re_instlist.instlist);	
	
	def = add_new_define(dst);
	usesrc = add_new_use(src, Opd);
	
	add_to_instlist(instnode, &re_instlist);

	re_resolve(&re_deflist, &re_uselist, &re_instlist);
	//print_info_of_current_inst(instnode);
}

void szconv_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
	re_list_t *entry;
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;
	valset_u  vt = {0};
	
	traverse_inst_operand(inst, src, dst, re_uselist, re_deflist, &nuse, &ndef);
	
	assert(nuse == 1 && ndef ==1);

	if (sign_of_valset(&(CAST2_USE(src[0]->node)->val), CAST2_USE(src[0]->node)->operand->datatype)) {
		one_valset(&vt);
	} else {
		zero_valset(&vt);
	}

	if (CAST2_USE(src[0]->node)->val_known 
			&& (CAST2_DEF(dst[0]->node)->val_stat & AfterKnown)){
		assert_val(dst[0], vt, false);
	}

	if (CAST2_USE(src[0]->node)->val_known 
			&& !(CAST2_DEF(dst[0]->node)->val_stat & AfterKnown)){

		assign_def_after_value(dst[0], vt);
		add_to_deflist(dst[0], re_deflist);
	}
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void szconv_vs_handler(re_list_t * instnode){
	re_list_t *dst[NOPD], *src[NOPD];
	int nuse, ndef;
	re_si si;

	vs_traverse_inst_operand(instnode, src, dst, &nuse, &ndef);
	assert(nuse == 1 && ndef == 1);

	if (list_empty(&CAST2_USE(src[0]->node)->valset.list)) {
		INIT_SI(si, 1, -1, 0);
		add_new_valset(&CAST2_DEF(dst[0]->node)->aft_valset,
				CONST_REGION, 0, si);
	} else {
		re_region_t * region;
		re_value_set *entry;

		region = get_region(&CAST2_USE(src[0]->node)->valset);
		assert(region->type == CONST_REGION);
		entry = list_first_entry(&CAST2_USE(src[0]->node)->valset.list,
					re_value_set, list);
		if (entry->si.lower_bound > 0) {
			INIT_SI(si, 0, 0, 0);
		} else if (entry->si.upper_bound < 0) {
			INIT_SI(si, 0, -1, -1);
		} else {
			INIT_SI(si, 1, -1, 0);
		}
		add_new_valset(&CAST2_DEF(dst[0]->node)->aft_valset,
				CONST_REGION, 0, si);
	}
}
#endif

void fpu_handler(re_list_t *instnode) {
}

void fpu_resolver(re_list_t* inst, re_list_t *re_deflist, re_list_t *re_uselist){
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
void fpu_vs_handler(re_list_t * instnode){
}
#endif

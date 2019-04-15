#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "global.h"
#include "disassemble.h"
#include "insthandler.h"
#include "reverse_exe.h"
#include "inst_opd.h"
#include "solver.h"
#include "bin_alias.h"
#include "re_valueset.h"
#include "re_runtime.h"
#include "inst_data.h"
#include "common.h"

static unsigned long truth_address(re_list_t *exp);

static unsigned long get_regvalue_from_pin(re_list_t *reg);

static bool is_effective_reg(re_list_t *entry);

static bool is_effective_exp(re_list_t *entry);

bool dl_region_verify_noalias(re_list_t *exp1, re_list_t *exp2) {
	int reg1 = GET_DL_REGION(exp1);
	int reg2 = GET_DL_REGION(exp2);

	if (reg1 != reg2)
		return true;
	else
		return false;
}

static void set_dl_region(re_list_t *entry, int region) {
// create one field in use_node_t and def_node_t to store dl_region from deeplearning

	region_type type;

	switch (region) {
	case 0:
		type = GLOBAL_REGION;
		break;
	case 1:
		type = HEAP_REGION;
		break;
	case 2:
		type = STACK_REGION;
		break;
	case 3:
		type = OTHER_REGION;
		break;
	default:
		assert("No Such Region Type" && 0);
		break;
	}

        if (entry->node_type == DefNode) {
		CAST2_DEF(entry->node)->dl_region = type;
        } else if (entry->node_type == UseNode) {
		CAST2_USE(entry->node)->dl_region = type;
        } else {
                assert("InstNode could not get here" && 0);
        }
}

void load_region_num(re_list_t *instnode, dlregion_list_t *dlreglist) {
        re_list_t *entry;
        size_t i, num = 0;
	re_list_t *memop[NOPD] = {NULL};

        //x86_insn_t * inst;
        //inst = re_ds.instlist + CAST2_INST(instnode->node)->inst_index;

        list_for_each_entry_reverse(entry, &instnode->list, list) {
                if ((entry->node_type == InstNode) ||
               	    (entry == &re_ds.head)) break;
		// get all the memory operands for current instruction
		if (is_effective_exp(entry)) memop[num++] = entry;
        }

        switch (dlreglist->dlreg_num){
                case 0:
                        break;
                case 1:
                        // only traverse the effective expressions
                        for (i = 0; i < num; i++) {
                                set_dl_region(memop[i], dlreglist->dlreg_list[0]);
                        }
                        break;
                case 2:
                        // only traverse the effective expressions
                        if (num == 2) {
                        	set_dl_region(memop[0], dlreglist->dlreg_list[1]);
                        	set_dl_region(memop[1], dlreglist->dlreg_list[0]);
			}
                        break;
                default:
                        assert(0);
        }
}

// load region in re_ds.dlregionlist and store them into re_valueset field
void load_region_from_DL() {
        re_list_t *entry;
        size_t index = 0;

	//LOG(stderr, "Load region from deep learning\n");
        list_for_each_entry(entry, &re_ds.head.list, list) {
                if (entry->node_type == InstNode){
                        index = CAST2_INST(entry->node)->inst_index;
                        load_region_num(entry, re_ds.dlregionlist + index);
                }
        }
}

#define GET_HT_ADDR(entry) \
	((entry)->node_type == DefNode) ? \
	(CAST2_DEF((entry)->node)->address) : \
	(CAST2_USE((entry)->node)->address) 

bool ht_verify_noalias(re_list_t *exp1, re_list_t *exp2) {
	unsigned addr1 = GET_HT_ADDR(exp1);
	unsigned addr2 = GET_HT_ADDR(exp2);

	if (!addr1 || !addr2)
		return false;
	else
		return (addr1 != addr2);
}


void noalias_pair_num_ht(re_list_t *exp, unsigned long *pt_num, unsigned long *ph_num, unsigned long *err_ph, unsigned long *pd_num, unsigned long *err_pd, unsigned long *hand_num) {
	// initialize the num of noalias pair
	*pt_num = 0;
	*ph_num = 0;
	*pd_num = 0;
	*hand_num = 0;
	*err_ph = 0;
	*err_pd = 0;

	re_list_t *entry;
	x86_op_t *opd;
	bool bool_truth, bool_ht, bool_dl;

	list_for_each_entry(entry, &exp->memlist, memlist) {
		if (entry == &re_ds.head) break;
		if (entry->node_type == UseNode && exp->node_type == UseNode) continue;
		//print_node(exp);
		//print_node(entry);
		bool_truth = truth_verify_noalias(entry, exp);
		bool_ht  = ht_verify_noalias(entry, exp);
		bool_dl  = dl_region_verify_noalias(entry, exp);
		// Truth: Alias, Value Set: No Alias
		if (!bool_truth && bool_ht) {
			print_node(exp);
			print_node(entry);
			(*err_ph)++;
		}
		// Truth: Alias, Value Set: No Alias, DeepLearning: No Alias
		if (!bool_truth && bool_ht && bool_dl) {
			print_node(exp);
			print_node(entry);
			(*err_pd)++;
		}

		if (bool_truth) (*pt_num)++;
		if (bool_dl && bool_ht) (*hand_num)++;
		if (bool_dl || bool_ht) (*pd_num)++;
		if (bool_ht)   (*ph_num)++;
	}
}

void ratio_noalias_pair_ht(){
	unsigned long long ht_noalias = 0, htdl_noalias = 0, truth_noalias = 0, hand_noalias = 0;
	unsigned long long ht_err_noalias = 0, htdl_err_noalias = 0;
	unsigned long htdl_err = 0, ht_err = 0;
	unsigned long tmp_ht = 0, tmp_truth = 0, tmp_htdl = 0, tmp_hand;
	//unsigned long memopd_num = 0;

	re_list_t *entry;
	x86_op_t *opd;
	
	list_for_each_entry(entry, &re_ds.head.memlist, memlist) {
		// print_node(entry);
		// initialize the value inside the function
		noalias_pair_num_ht(entry, &tmp_truth, &tmp_ht, &ht_err, &tmp_htdl, &htdl_err, &tmp_hand);
		//LOG(stdout, "temp valset_noalias %ld ", tmp_vs);
		//LOG(stdout, "temp truth_noalias %ld\n", tmp_truth);
		ht_noalias += tmp_ht;
		hand_noalias += tmp_hand;
		htdl_noalias += tmp_htdl;
		truth_noalias += tmp_truth;
		ht_err_noalias += ht_err;
		htdl_err_noalias += htdl_err;
		//LOG(stdout, "valset_noalias %ld ", valset_noalias);
		//LOG(stdout, "truth_noalias %ld\n", truth_noalias);
		//memopd_num++;
	}

	LOG(stdout, "~~~~~~~~~~~~~~~~~~~Result of No Alias Pair (HT)~~~~~~~~~~~~~~~~~~~\n");
	//LOG(stdout, "Total Memory Operand Number is %ld\n", memopd_num);
	LOG(stdout, "Total Hypothesis Testing No Alias Pair is %lld\n", ht_noalias);
	LOG(stdout, "Total HT & DL Analysis No Alias Pair is %lld\n", hand_noalias);
	LOG(stdout, "Total HT + DL Analysis No Alias Pair is %lld\n", htdl_noalias);
	LOG(stdout, "Total Ground Truth No Alias Pair is %lld\n", truth_noalias);
	//LOG(stdout, "Total Error Number for Valset Analysis is %lld\n", hta_err_noalias);
	//LOG(stdout, "Total Error Number for Valset + DL Analysis is %lld\n", dl_err_noalias);
	LOG(stdout, "No Alias Pair Percent for HT is %lf\n", ((double)ht_noalias)/truth_noalias);
	LOG(stdout, "No Alias Pair Percent for HT & DL is %lf\n", ((double)hand_noalias)/truth_noalias);
	LOG(stdout, "No Alias Pair Percent for HT + DL is %lf\n", ((double)htdl_noalias)/truth_noalias);
	LOG(stdout, "No Alias Pair Error Percent for HT is %lf\n", ((double)ht_err_noalias)/truth_noalias);
	LOG(stdout, "No Alias Pair Error Percent for HT + DL is %lf\n", ((double)htdl_err_noalias)/truth_noalias);
}

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)

#define VALSET_HEU_SIZE 80
#define ITEMDEM "-"
#define INFODEM ":"

void init_vsa_analysis() {
	
	//check_offset_type();

	// find the value of first esp
	re_ds.esp_value = find_value_of_first_esp();
	LOG(stdout, "Value of the first esp is %lx\n", re_ds.esp_value);

	memset(&re_ds.valset_heu, 0, sizeof(valset_heu_t));

	load_valset_heuristic();

	init_region_info();
}

static void process_valset_heu(char *line, unsigned index);

void load_valset_heuristic() {
	
	char log_buf[VALSET_HEU_SIZE];
	FILE* file; 
	unsigned index; 

	if((file = fopen(get_heu_path(), "r")) == NULL){
		return;
	}
	
	index = 0;
	memset(log_buf, 0, VALSET_HEU_SIZE);
	
	while(fgets(log_buf, sizeof(log_buf), file) != 0){
		process_valset_heu(log_buf, index);
		index++;
	}
}

static void process_valset_heu(char * line, unsigned index) {
	//LOG(stdout, "line %d is %s\n", index, line);
	char *str1, *str2, *saveptr1, *saveptr2; 
	int icount, pic_count;
	heu_type type;
	char *token, *start, *endptr;

	for(icount = 0, str1 = line; ; icount++, str1 = NULL){
		token = strtok_r(str1, ITEMDEM, &saveptr1);
		if(token == NULL) break; 	

		//The first item represents the type
		if (icount == 0) {
			if (strcmp(token, "malloc") == 0) {
				type = HEU_MALLOC;
				continue; 
			}
			if (strcmp(token, "global") == 0) {
				type = HEU_GLOBAL;
				continue; 
			}
			assert(0 && "The summary type is not correct\n");	
		}

		switch (type) {
		case HEU_MALLOC:
			start = strtok_r(token, INFODEM, &saveptr2);
			if (start) {
				re_ds.valset_heu.malloc_addrs[MALLOC] = strtoll(start, &endptr, 16);
			}
			start = strtok_r(NULL, INFODEM, &saveptr2);
			if (start) {
				re_ds.valset_heu.malloc_addrs[FREE] = strtoll(start, &endptr, 16);
			}
			start = strtok_r(NULL, INFODEM, &saveptr2);
			if (start) {
				re_ds.valset_heu.malloc_addrs[CALLOC] = strtoll(start, &endptr, 16);
			}
			start = strtok_r(NULL, INFODEM, &saveptr2);
			if (start) {
				re_ds.valset_heu.malloc_addrs[REALLOC] = strtoll(start, &endptr, 16);
			}

			for (icount = 1; icount < MALLOC_FUNC_NUM; icount++) {
				LOG(stdout, "malloc 0x%lx\n", re_ds.valset_heu.malloc_addrs[icount]);
			}

			break;
		case HEU_GLOBAL:
			for (pic_count=0; ; pic_count++, token = NULL) {
				start = strtok_r(token, INFODEM, &saveptr2);
				if (!start) break;

				re_ds.valset_heu.get_pc_func[pic_count] = strtoll(start, &endptr, 16);
			}
			
			assert(pic_count <= GET_PC_NUM);
			re_ds.valset_heu.pic_count = pic_count;

			LOG(stdout, "get_pc_func[0] 0x%lx\n", re_ds.valset_heu.get_pc_func[0]);
			LOG(stdout, "get_pc_func[1] 0x%lx\n", re_ds.valset_heu.get_pc_func[1]);

			break;
		default:
			assert(0 && "Other type of heu");
		}
	}
}

static mal_family_t check_malloc_funcs(uint32_t addr) {
	if (addr == re_ds.valset_heu.malloc_addrs[MALLOC]) {
		return MALLOC;	
	}
	if (addr == re_ds.valset_heu.malloc_addrs[FREE]) {
		return FREE;	
	}
	if (addr == re_ds.valset_heu.malloc_addrs[CALLOC]) {
		return CALLOC;	
	}
	if (addr == re_ds.valset_heu.malloc_addrs[REALLOC]) {
		return REALLOC;	
	}
	return NONE;
}

void search_heap_heu() {
	// traverse all the function information and check all malloc family functions, then count them.
	int index = 0, inst_index;
	int malloc_site = 0;
	re_list_t *entry, *ret_inst, *ret_value;
	re_region_t *region;
	re_si si;

	for (index = 0; index <= re_ds.maxfuncid; index++) {
		inst_index = re_ds.flist[index].end;
		entry = get_entry_by_inst_id(inst_index);	
		switch (check_malloc_funcs(re_ds.instlist[inst_index].addr)) {
		case MALLOC:
		case CALLOC:
		case REALLOC:
			// add Heap Region
			region = select_newest_region(HEAP_REGION);
			if (!region) {
				region = add_new_region(HEAP_REGION, 0);
			} else {
				region = add_new_region(HEAP_REGION, region->sub_id + 1);
			}
			print_info_of_current_inst(entry);
			LOG(stdout, "inst index is %d\n", inst_index);
			malloc_site++;

			ret_inst = get_entry_by_inst_id(re_ds.flist[index].start);	
			ret_value = find_return_value(ret_inst);
			
			INIT_SI(si, 0, 0, 0);
			add_new_valset(&CAST2_DEF(ret_value->node)->aft_valset, 
				region->type, region->sub_id, si);	
			print_info_of_current_inst(entry);
			break;
		default:
			break;
		}
		
	}
	LOG(stdout, "malloc site is %d\n", malloc_site);
}

void search_pic_global_heu() {
	// traverse all the function information and check all get_pc functions, then count them.
	int index = 0, get_pc_index = 0, inst_index;
	int addr;
	int get_pc_site = 0;
	re_list_t *entry, *inst_offset;
	re_region_t *region[GET_PC_NUM];
	re_si si;

	re_list_t *use[NOPD], *def[NOPD];
	size_t nuse, ndef;

	for (index = 0; index < re_ds.valset_heu.pic_count; index++) { 
		// add Global Region
		region[index] = select_newest_region(GLOBAL_REGION);
		if (!region[index]) {
			region[index] = add_new_region(GLOBAL_REGION, 0);
		} else {
			region[index] = add_new_region(GLOBAL_REGION, region[index]->sub_id + 1);
		}
	}
	for (index = 0; index <= re_ds.maxfuncid; index++) {
		inst_index = re_ds.flist[index].end;
		entry = get_entry_by_inst_id(inst_index);	
		addr = re_ds.instlist[inst_index].addr;
		for (get_pc_index = 0; get_pc_index < re_ds.valset_heu.pic_count; get_pc_index++) { 
			if (addr == re_ds.valset_heu.get_pc_func[get_pc_index]) {
				//print_info_of_current_inst(entry);
				//LOG(stdout, "inst index is %d\n", inst_index);
				get_pc_site++;

				// mov ebx, [esp]	
				// ......
				// add ebx, XXX
				// the after value of ebx is Global Region
				inst_offset = get_entry_by_inst_id(CAST2_INST(entry->node)->inst_index - 2);

				obtain_inst_elements(inst_offset, use, def, &nuse, &ndef);

				INIT_SI(si, 0, 0, 0);
				add_new_valset(&CAST2_DEF(def[0]->node)->aft_valset, 
						region[get_pc_index]->type,
						region[get_pc_index]->sub_id, si);	
				//print_info_of_current_inst(inst_offset);
			}
		}
	}
	LOG(stdout, "get_pc_site is %d\n", get_pc_site);
}

void init_region_info() {

	// first record all the segments(memory regions), property - base address, size;
	int max_fid = maxfuncid();
	LOG(stdout, "Max Function Id is %d\n", max_fid);

	re_ds.maxfuncid = max_fid;

	adjust_stack_boundary();

	//print_func_info(&re_ds.head);

	INIT_LIST_HEAD(&re_ds.region_head.list);

	// add Global Region
	add_new_region(GLOBAL_REGION, 0);

	// add Heap Region, but this region only works just as one symbol
	add_new_region(HEAP_REGION, 0);

	// add Stack Region, use one esp and its offset to address
	add_new_region(STACK_REGION, 0);

	// add GS Region
	add_new_region(GS_REGION, 0);

	// add Register Region
	add_new_region(REG_REGION, 0);

	// add Constant Region
	add_new_region(CONST_REGION, 0);

	// add Heap Region, but this region only works just as one symbol
	add_new_region(OTHER_REGION, 0);

	init_first_esp();

	search_heap_heu();

	search_pic_global_heu();

	print_region_info(&re_ds.region_head);
}

// FIXME: create new struct to show Bool Alias (later)
bool valset_verify_noalias(re_list_t *entry, re_list_t *target) {
	x86_op_t *opd;
	size_t size1, size2;
	re_value_set *addr_entry, *addr_target;

	opd = GET_OPERAND(entry);
	size1 = translate_datatype_to_byte(opd->datatype);

	opd = GET_OPERAND(target);
	size2 = translate_datatype_to_byte(opd->datatype);

	addr_entry = GET_ADDR_VALSET(entry);
	addr_target = GET_ADDR_VALSET(target);
	
	return (!mem_valset_overlap(addr_entry, size1, addr_target, size2));
}

void noalias_pair_num_vsa(re_list_t *exp, unsigned long *pt_num, unsigned long *pv_num, unsigned long *err_pv, unsigned long *pd_num, unsigned long *err_pd) {
	// initialize the num of noalias pair
	*pt_num = 0;
	*pv_num = 0;
	*pd_num = 0;
	*err_pv = 0;
	*err_pd = 0;

	re_list_t *entry, *tmp;
	x86_op_t *opd;
	bool bool_truth, bool_vs, bool_dl;

	list_for_each_entry(entry, &exp->memlist, memlist) {
		if (entry == &re_ds.head) break;
		if (entry->node_type == UseNode && exp->node_type == UseNode) continue;
		//print_node(exp);
		//print_node(entry);

		tmp = find_inst_of_node(entry);
		if (CAST2_INST(tmp->node)->inst_index > 0){
			break;

		bool_truth = truth_verify_noalias(entry, exp);
		bool_vs  = valset_verify_noalias(entry, exp);
		bool_dl  = dl_region_verify_noalias(entry, exp);
		// Truth: Alias, Value Set: No Alias
		if (!bool_truth && bool_vs) {
			print_node(exp);
			print_node(entry);
			(*err_pv)++;
		}
		// Truth: Alias, Value Set: No Alias, DeepLearning: No Alias
		if (!bool_truth && bool_vs && bool_dl) {
			print_node(exp);
			print_node(entry);
			(*err_pd)++;
		}

		if (bool_truth) (*pt_num)++;
		if (bool_dl || bool_vs) (*pd_num)++;
		if (bool_vs)   (*pv_num)++;
	}
}

void ratio_noalias_pair_vsa(){
	unsigned long long vs_noalias = 0, vsdl_noalias = 0, truth_noalias = 0;
	unsigned long long vs_err_noalias = 0, vsdl_err_noalias = 0;
	unsigned long vsdl_err = 0, vs_err = 0;
	unsigned long tmp_vs = 0, tmp_truth = 0, tmp_vsdl = 0;
	//unsigned long memopd_num = 0;

	re_list_t *entry, *tmp;
	x86_op_t *opd;
	
	list_for_each_entry(entry, &re_ds.head.memlist, memlist) {
		tmp = find_inst_of_node(entry);
		if (CAST2_INST(tmp->node)->inst_index > 0){
			break;
		}
		// print_node(entry);
		// initialize the value inside the function
		noalias_pair_num_vsa(entry, &tmp_truth, &tmp_vs, &vs_err, &tmp_vsdl, &vsdl_err);
		//LOG(stdout, "temp valset_noalias %ld ", tmp_vs);
		//LOG(stdout, "temp truth_noalias %ld\n", tmp_truth);
		vs_noalias += tmp_vs;
		vsdl_noalias += tmp_vsdl;
		truth_noalias += tmp_truth;
		vs_err_noalias += vs_err;
		vsdl_err_noalias += vsdl_err;
		//LOG(stdout, "valset_noalias %ld ", valset_noalias);
		//LOG(stdout, "truth_noalias %ld\n", truth_noalias);
		//memopd_num++;
	}

	LOG(stdout, "~~~~~~~~~~~~~~~~~~~Result of No Alias Pair (VSA)~~~~~~~~~~~~~~~~~~~\n");
	//LOG(stdout, "Total Memory Operand Number is %ld\n", memopd_num);
	LOG(stdout, "Total Valset Analysis No Alias Pair is %lld\n", vs_noalias);
	LOG(stdout, "Total Valset + DL Analysis No Alias Pair is %lld\n", vsdl_noalias);
	LOG(stdout, "Total Ground Truth No Alias Pair is %lld\n", truth_noalias);
	//LOG(stdout, "Total Error Number for Valset Analysis is %lld\n", vsa_err_noalias);
	//LOG(stdout, "Total Error Number for Valset + DL Analysis is %lld\n", dl_err_noalias);
	LOG(stdout, "No Alias Pair Percent for VSA is %lf\n", ((double)vs_noalias)/truth_noalias);
	LOG(stdout, "No Alias Pair Percent for VSA + DL is %lf\n", ((double)vsdl_noalias)/truth_noalias);
	LOG(stdout, "No Alias Pair Error Percent for VSA is %lf\n", ((double)vs_err_noalias)/truth_noalias);
	LOG(stdout, "No Alias Pair Error Percent for VSA + DL is %lf\n", ((double)vsdl_err_noalias)/truth_noalias);
}

void valset_statistics(){
	long entry_num = 0, mem_num = 0, reg_num = 0;
	long addr_known_num = 0; // only for memory access
	long reg_valknown_num = 0;
	long mem_valknown_num = 0;
	re_list_t *entry;
	x86_op_t *opd;
	list_for_each_entry_reverse(entry, &re_ds.head.list, list) {
		if (entry->node_type == InstNode) continue;

		entry_num++;

		opd = GET_OPERAND(entry);

		if (entry->node_type == DefNode) {
			if (opd->type == op_expression) {
				mem_num++;
				if (!list_empty(&CAST2_DEF(entry->node)->addr_valset.list)) {
					addr_known_num++;
				}
				if (!list_empty(&CAST2_DEF(entry->node)->bef_valset.list) ||
				    !list_empty(&CAST2_DEF(entry->node)->aft_valset.list)) {
					mem_valknown_num++;
				}
			}
			if (opd->type == op_register) {
				reg_num++;
				if (!list_empty(&CAST2_DEF(entry->node)->bef_valset.list) ||
				    !list_empty(&CAST2_DEF(entry->node)->aft_valset.list)) {
					reg_valknown_num++;
				}
			}
		}

		if (entry->node_type == UseNode) {
			if ((CAST2_USE(entry->node)->usetype == Opd) &&
			    (opd->type == op_expression)) {
				mem_num++;
				if (!list_empty(&CAST2_USE(entry->node)->addr_valset.list)) {
					addr_known_num++;
				}
				if (!list_empty(&CAST2_USE(entry->node)->valset.list)) {
					mem_valknown_num++;
				}
			}
			if ((CAST2_USE(entry->node)->usetype != Opd) ||
			    ((CAST2_USE(entry->node)->usetype == Opd) &&
			     (opd->type == op_register))) {
				reg_num++;
				if (!list_empty(&CAST2_USE(entry->node)->valset.list)) {
					reg_valknown_num++;
				}
			}
		}
	}
	LOG(stdout, "~~~~~~~~~~~~~~~~~~~~~Result of Value Set~~~~~~~~~~~~~~~~~~~~~\n");
	LOG(stdout, "Total Entry Num is %ld\n", entry_num);
	LOG(stdout, "Total Memory Num is %ld\n", mem_num);
	LOG(stdout, "Total Register Num is %ld\n", reg_num);
	LOG(stdout, "Total Address Known Num is %ld\n", addr_known_num);
	LOG(stdout, "Address Known Average is %f\n", ((float)addr_known_num)/mem_num);
	LOG(stdout, "Total Register Value Known Num is %ld\n", reg_valknown_num);
	LOG(stdout, "Reg Value Known Average is %f\n", ((float)reg_valknown_num)/reg_num);
	LOG(stdout, "Total Memory Value Known is %ld\n", mem_valknown_num);
	LOG(stdout, "Mem Value Known Average is %f\n", ((float)mem_valknown_num)/mem_num);
}

void valset_correctness_check() {

	// directly go through entry in the linkedlist
	re_list_t *entry;
	re_value_set *vs_entry;
	re_value_set *vs_addr;
	x86_op_t *opd;
	unsigned long pic_g1 = 0, pic_g2 = 0;
	unsigned long addr_entry;

	list_for_each_entry(entry, &re_ds.head.list, list) {
		if (entry->node_type == InstNode) continue;
		
		if (is_effective_exp(entry)) {
			//print_node(entry);
			vs_addr = GET_ADDR_VALSET(entry);
			//only check the correctness of memory
			list_for_each_entry(vs_entry, &vs_addr->list, list) {
				switch(vs_entry->region->type) {
				// now just sub different base address
				case STACK_REGION:
					addr_entry = GET_TRUE_ADDR(entry);
					//LOG(stdout, "addr_entry %lx first esp %lx offset %ld\n",
					//	addr_entry, esp_value, addr_entry-esp_value);
					assert(IS_IN_SI(addr_entry - re_ds.esp_value, &vs_entry->si));
					break;
				case GS_REGION:
					addr_entry = GET_TRUE_ADDR(entry);
					//LOG(stdout, "addr_entry %lx addr_entry %ld\n",
					//	addr_entry, addr_entry);
					assert(IS_IN_SI(addr_entry - re_ds.coredata->corereg.gs_base, &vs_entry->si));
					break;
				case GLOBAL_REGION:
					addr_entry = GET_TRUE_ADDR(entry);
					//LOG(stdout, "addr_entry %lx addr_entry %ld\n",
					//	addr_entry, addr_entry);
					if (vs_entry->region->sub_id == 0) {
						if (!si_is_full(&vs_entry->si)) {
							addr_entry = GET_TRUE_ADDR(entry);
							//LOG(stdout, "addr_entry %lx addr_entry %ld\n",
							//	addr_entry, addr_entry);
							assert(IS_IN_SI(addr_entry, &vs_entry->si));
						}
					} else if (vs_entry->region->sub_id == 1) {
						if (pic_g1 == 0) {
							if (si_is_one_value(&vs_entry->si)) {
								pic_g1 = addr_entry - vs_entry->si.lower_bound;
							}
						} else {
							assert(IS_IN_SI(addr_entry - pic_g1, &vs_entry->si));
						}
					} else if (vs_entry->region->sub_id == 2) {
						if (pic_g2 == 0) {
							assert(vs_entry->si.stride == 0);
							pic_g2 = addr_entry - vs_entry->si.lower_bound;
						} else {
							assert(IS_IN_SI(addr_entry - pic_g2, &vs_entry->si));
						}
					} else {
						assert(0 && "No other Global Region");
					}
					break;
				case HEAP_REGION:
					#warning heap region heuristic
					break;
				case CONST_REGION:
					// directly set address in the assembly code
					LOG(stdout, "constant region in memory\n");
					addr_entry = GET_TRUE_ADDR(entry);

					LOG(stdout, "addr_entry %lx , si ", addr_entry);
					print_strided_interval(&vs_entry->si);
					LOG(stdout, "\n");

					if (!IS_IN_SI(addr_entry, &vs_entry->si)) {
						LOG(stdout, "warning : addr_entry %lx\n", addr_entry);
						assert(addr_entry > 0x8000000);
					}
					break;
				case OTHER_REGION:
					break;
				default:
					LOG(stderr, "Region Type : %d\n", vs_entry->region->type);
					break;
				}
			}
		}
	}
}

void assign_use_value_set(re_list_t *use, re_value_set *valset) {
	//print_valset(valset);

	// if valset is not empty, just return 
	if (!list_empty(&CAST2_USE(use->node)->valset.list))
		return;

	fork_value_set(&CAST2_USE(use->node)->valset, valset);
}

void assign_def_before_value_set(re_list_t *def, re_value_set *valset) {
	//print_valset(valset);

	// if valset is not empty, just return 
	if (!list_empty(&CAST2_DEF(def->node)->bef_valset.list))
		return;

	fork_value_set(&CAST2_DEF(def->node)->bef_valset, valset);
}

void assign_def_after_value_set(re_list_t *def, re_value_set *valset) {
	//print_valset(valset);

	// if valset is not empty, just return 
	if (!list_empty(&CAST2_DEF(def->node)->aft_valset.list))
		return;

	fork_value_set(&CAST2_DEF(def->node)->aft_valset, valset);
}

static cmp_result vs_compare_def_def(re_list_t *first, re_list_t *second) {
	def_node_t *fd = (def_node_t*) first->node;
	def_node_t *sd = (def_node_t*) second->node;

	size_t size1, size2;

	size1 = translate_datatype_to_byte(fd->operand->datatype);
	size2 = translate_datatype_to_byte(sd->operand->datatype);

	if(fd->operand->type != sd->operand->type) {
		return NO_MATCH;
	}

	if(fd->operand->type == op_register) {
		if (exact_same_regs(fd->operand->data.reg,
				    sd->operand->data.reg)) {
			return EXACT;
		}

		if (reg1_alias_reg2(fd->operand->data.reg,
				    sd->operand->data.reg)) {
			return SUB;
		}

		if (reg1_alias_reg2(sd->operand->data.reg,
				    fd->operand->data.reg)) {
			return SUPER;
		}

		if (same_alias(fd->operand->data.reg,
			       sd->operand->data.reg)) {
			if (fd->operand->data.reg.size ==
			    sd->operand->data.reg.size) {
				return NO_MATCH;
			}

			if (fd->operand->data.reg.size >
			    sd->operand->data.reg.size) {
				return SUPER;
			}
			return SUB;
		}
	}
	if (fd->operand->type == op_expression){
		if ( list_empty(&fd->addr_valset.list) ||
		     list_empty(&sd->addr_valset.list)) {
			return NO_MATCH;
		}

		if (mem_valset_exact(&fd->addr_valset, size1,
				     &sd->addr_valset, size2)) {
			return EXACT;
		}

		if (mem_valset_subst(&fd->addr_valset, size1,
				     &sd->addr_valset, size2)) {
			return SUB;
		}

		if (mem_valset_super(&fd->addr_valset, size1,
				     &sd->addr_valset, size2)) {
			return SUPER;
		}

		if (mem_valset_overlap(&fd->addr_valset, size1,
				       &sd->addr_valset, size2)) {
			return OVERLAP;
		}
	}
	if (fd->operand->type == op_offset) {
		assert(0 && "this type should be changed to expression");
	}
	return NO_MATCH;
}

static cmp_result vs_compare_def_use(re_list_t *first, re_list_t *second) {
	def_node_t *fd = (def_node_t *)first->node;
	use_node_t *su = (use_node_t *)second->node;

	size_t size1, size2;

	size1 = translate_datatype_to_byte(fd->operand->datatype);
	size2 = translate_datatype_to_byte(su->operand->datatype);

	if (su->usetype == Opd) {
		if (fd->operand->type != su->operand->type){
			return NO_MATCH;
		}

		if(fd->operand->type == op_register){
			if (exact_same_regs(fd->operand->data.reg,
					    su->operand->data.reg)) {
				return EXACT;
			}

			if (reg1_alias_reg2(fd->operand->data.reg,
					    su->operand->data.reg)) {
				return SUB;
			}

			if (reg1_alias_reg2(su->operand->data.reg,
					    fd->operand->data.reg)) {
				return SUPER;
			}

			if (same_alias(fd->operand->data.reg,
				       su->operand->data.reg)) {
				if (fd->operand->data.reg.size ==
				    su->operand->data.reg.size) {
					return NO_MATCH;
				}

				if (fd->operand->data.reg.size >
				    su->operand->data.reg.size) {
					return SUPER;
				}
				return SUB;
			}
		}
		if (fd->operand->type == op_expression) {
			if ( list_empty(&fd->addr_valset.list) ||
			     list_empty(&su->addr_valset.list)) {
				return NO_MATCH;
			}

			if (mem_valset_exact(&fd->addr_valset, size1,
					     &su->addr_valset, size2)) {
				return EXACT;
			}
			
			if (mem_valset_subst(&fd->addr_valset, size1,
					     &su->addr_valset, size2)) {
				return SUB;
			}

			if (mem_valset_super(&fd->addr_valset, size1,
					     &su->addr_valset, size2)) {
				return SUPER;
			}

			if (mem_valset_overlap(&fd->addr_valset, size1,
					       &su->addr_valset, size2)) {
				return OVERLAP;
			}
		}
		if (fd->operand->type == op_offset) {
			assert(0 && "this type should be changed to expression");
		}
	} else if (su->usetype == Base) {
		if (fd->operand->type != op_register){
			return NO_MATCH;
		}

		// Base is always 32 bit register in x86
		if (exact_same_regs(fd->operand->data.reg,
				su->operand->data.expression.base)) {
			return EXACT;
		}
		if (reg1_alias_reg2(fd->operand->data.reg,
				su->operand->data.expression.base)) {
			return SUB;
		}
	} else if (su->usetype == Index) {
		if (fd->operand->type != op_register){
			return NO_MATCH;
		}

		// Base is always 32 bit register in x86
		if (exact_same_regs(fd->operand->data.reg,
				su->operand->data.expression.index)) {
			return EXACT;
		}
		if (reg1_alias_reg2(fd->operand->data.reg,
				su->operand->data.expression.index)) {
			return SUB;
		}
	} else {
		assert(0 && "never get here");
	}
	return NO_MATCH;
}

static cmp_result vs_compare_use_use(re_list_t *first, re_list_t *second) {
	assert(0);
}

static void reverse_cmp_result(cmp_result *type) {
	switch (*type) {
	case SUB:
		(*type) = SUPER;
		break;
	case SUPER:
		(*type) = SUB;
		break;
	case EXACT:
	case OVERLAP:
		break;
	}
}

cmp_result vs_compare_two_targets(re_list_t *first, re_list_t *second) {
	cmp_result type;

	if(first->node_type == DefNode
			&& second ->node_type == DefNode){
		return vs_compare_def_def(first, second);
	}
	if(first->node_type == DefNode
			&& second ->node_type == UseNode){

		return vs_compare_def_use(first, second);
	}

	if(first->node_type == UseNode
			&& second ->node_type == DefNode){

		type = vs_compare_def_use(second, first);
		reverse_cmp_result(&type);
		return type;
	}

	if(first->node_type == UseNode
			&& second ->node_type == UseNode){
		return vs_compare_use_use(first, second);
	}
}

re_list_t * vs_find_prev_def_of_def(re_list_t *def, cmp_result *type) __attribute__ ((alias("vs_find_prev_def_of_use")));

re_list_t * vs_find_prev_use_of_def(re_list_t *def, cmp_result *type) __attribute__ ((alias("vs_find_prev_use_of_use")));

re_list_t * vs_find_prev_def_of_use(re_list_t *use, cmp_result *type) {

	re_list_t *entry;

	list_for_each_entry_reverse(entry, &use->list, list) {
		if (entry == &re_ds.head) break;
		if (entry->node_type != DefNode)
			continue;

		*type = vs_compare_two_targets(entry, use);
		if (*type) return entry;
	}
	return NULL;

}

re_list_t * vs_find_prev_use_of_use(re_list_t *use, cmp_result *type) {

	re_list_t *entry;

	list_for_each_entry_reverse(entry, &use->list, list) {
		if (entry == &re_ds.head) break;
		if (entry->node_type != UseNode)
			continue;

		*type = vs_compare_two_targets(entry, use);
		if (*type) return entry;
	}
	return NULL;

}

void init_first_esp() {
	x86_op_t *opd;
	re_si si;
	re_list_t *entry;
	int funcid = 0;
	list_for_each_entry(entry, &re_ds.head.list, list) {
		if (entry == &re_ds.head) break;

		if (entry->node_type == DefNode) {
			opd = CAST2_DEF(entry->node)->operand;
			if (x86_opd_is_esp(opd)) {
				// set esp before value
				INIT_SI(si, 0, 0, 0);
				add_new_valset(&(CAST2_DEF(entry->node)->bef_valset),
					STACK_REGION, funcid, si);
				break;
			}
		}

		if (entry->node_type == UseNode) {
			opd = CAST2_USE(entry->node)->operand;
			if (((CAST2_USE(entry->node)->usetype == Base)&&
			     (x86_base_is_esp(opd))) ||
			    ((CAST2_USE(entry->node)->usetype == Index)&&
			     (x86_index_is_esp(opd))) ||
			    ((CAST2_USE(entry->node)->usetype == Opd) &&
			     (x86_opd_is_esp(opd)))) {
				INIT_SI(si, 0, 0, 0);
				add_new_valset(&(CAST2_USE(entry->node)->valset),
					STACK_REGION, funcid, si);
			}
		}
	}
}

unsigned long find_value_of_first_esp(){
	x86_op_t *opd;
	re_list_t *entry;
	list_for_each_entry(entry, &re_ds.head.list, list) {
		if (entry == &re_ds.head) break;

		if (entry->node_type == DefNode) {
			opd = CAST2_DEF(entry->node)->operand;
			if (x86_opd_is_esp(opd)) break;
		}

		if (entry->node_type == UseNode) {
			opd = CAST2_USE(entry->node)->operand;
			if (((CAST2_USE(entry->node)->usetype == Base)&&
			     (x86_base_is_esp(opd))) ||
			    ((CAST2_USE(entry->node)->usetype == Index)&&
			     (x86_index_is_esp(opd))) ||
			    ((CAST2_USE(entry->node)->usetype == Opd) &&
			     (x86_opd_is_esp(opd)))) {
				break;
			}
		}
	}
	//print_node(entry);	
	return get_regvalue_from_pin(entry);
}

void vs_res_reg_in_exp(re_list_t *entry) {
	cmp_result type;
	re_si si;
	x86_op_t *opd;

	re_list_t *prevdef;

	opd = CAST2_USE(entry->node)->operand;

	if (CAST2_USE(entry->node)->usetype == Index) {
		// Index
		//LOG(stdout, "add new valset to index\n");
		//print_node(index);
		INIT_SI(si, 0, opd->data.expression.index.id,
			       opd->data.expression.index.id);
		add_new_valset(&CAST2_USE(entry->node)->addr_valset,
				REG_REGION, 0, si);
	}

	if (CAST2_USE(entry->node)->usetype == Base) {
		// Base
		//LOG(stdout, "add new valset to base\n");
		//print_node(base);
		INIT_SI(si, 0, opd->data.expression.base.id,
			       opd->data.expression.base.id);
		add_new_valset(&CAST2_USE(entry->node)->addr_valset,
				REG_REGION, 0, si);
	}

	prevdef = vs_find_prev_def_of_use(entry, &type);
	if (!prevdef) {
		//LOG(stdout, "No previous define\n");
		// if no previous define, maybe truncated or ...
	} else {
		if((type == EXACT) || (type == SUPER))
			assign_use_value_set(entry,
				&(CAST2_DEF(prevdef->node)->aft_valset));
	}
}

static void vs_init_reg_valset(re_list_t *entry) {
	x86_op_t *opd;
	re_si si;
	re_value_set *head;

	head = GET_ADDR_VALSET(entry);
	opd = GET_OPERAND(entry);

	if (entry->node_type == DefNode) {
		INIT_SI(si, 0, opd->data.reg.id, opd->data.reg.id);
		add_new_valset(&CAST2_DEF(entry->node)->addr_valset, REG_REGION, 0, si);
	}
	
	if (entry->node_type == UseNode){
		INIT_SI(si, 0, opd->data.reg.id, opd->data.reg.id);
		add_new_valset(&CAST2_USE(entry->node)->addr_valset, REG_REGION, 0, si);
	}
}

// resolve other operands than memory access
void vs_res_non_expression(re_list_t *entry) {

	x86_op_t *opd;
	re_list_t *prevdef;
	cmp_result type;
	re_si si;

	opd = GET_OPERAND(entry);

	if (opd->type == op_immediate) {
		INIT_SI(si, 0, CAST2_USE(entry->node)->val.dword,
				CAST2_USE(entry->node)->val.dword);
		add_new_valset(&CAST2_USE(entry->node)->valset,
				CONST_REGION, 0, si);
		return;
	}	

	if (!x86_opd_is_register(opd)) {
		print_node(entry);
		assert(0 && "operand is not register type");
	}

	vs_init_reg_valset(entry);

	if (entry->node_type == DefNode) {

		prevdef = vs_find_prev_def_of_def(entry, &type);

		if ((prevdef) && ((type == EXACT) || (type == SUPER))) {
			assign_def_before_value_set(entry,
				&(CAST2_DEF(prevdef->node)->aft_valset));
		}
	} else {

		prevdef = vs_find_prev_def_of_use(entry, &type);

		if ((prevdef) && ((type == EXACT) || (type == SUPER))) {
			assign_use_value_set(entry,
				&(CAST2_DEF(prevdef->node)->aft_valset));
		}
	}
}


// in one intel x86 memory access, only one register indicts its region
void vs_res_expression(re_list_t *exp) {

	re_list_t *index, *base;
	re_list_t *prevdef;
	x86_op_t *opd;
	re_value_set *addr_valset;
	re_region_t *region;
	re_value_set tmp;
	re_value_set *entry;
	cmp_result type;
	re_si si;

	INIT_LIST_HEAD(&tmp.list);

	index = NULL;
	base = NULL;

	opd = GET_OPERAND(exp);
	addr_valset = GET_ADDR_VALSET(exp);

	// If the address is set, just return
	if (!list_empty(&addr_valset->list)) return;

	get_element_of_exp(exp, &index, &base);

	if (index) {
		//LOG(stdout, "resolve index register in expression:\n");
		//print_node(index);
		vs_res_reg_in_exp(index);
	}
	if (base) {
		//LOG(stdout, "resolve base register in expression:\n");
		//print_node(base);
		vs_res_reg_in_exp(base);
	}

	// just get memory address value set from registers
	// region id can be obtained by base register
	// concrete offset can be obtained by base and index register
	// special when gs and global

	// get strided interval of region_id, add together
	// CAST2_DEF(exp->node)->addr_valset;
	// CAST2_USE(base->node)->valset;
	// CAST2_USE(index->node)->valset;
	// opd->data.expression.disp;
	if (op_with_gs_seg(opd)) {
		if (!base && !index) {
			INIT_SI(si, 0, opd->data.expression.disp,
				opd->data.expression.disp);
			add_new_valset(addr_valset,
				GS_REGION, 0, si);
		} else if (base && !index) {
			if (list_empty(&CAST2_USE(base->node)->valset.list)) {
				si_set_full(&si);
				add_new_valset(addr_valset, 
					GS_REGION, 0, si);
			} else {
				valset_add_offset(addr_valset,
					&CAST2_USE(base->node)->valset,
					opd->data.expression.disp);
				list_for_each_entry(entry, &addr_valset->list, list) {
					valset_modify_region(entry, GS_REGION, 0);
				}
			}
		} else if (!base && index) {
			assert(0 && "Index");
		} else {
			assert(0 && "Base and Index");
		}
	} else {
		if (!base && !index) {
			INIT_SI(si, 0, opd->data.expression.disp,
				opd->data.expression.disp);
			add_new_valset(addr_valset,
				GLOBAL_REGION, 0, si);
		} else if (base && !index) {
			valset_add_offset(addr_valset,
				&CAST2_USE(base->node)->valset,
				opd->data.expression.disp);
		} else if (!base && index) {
			if (list_empty(&CAST2_USE(index->node)->valset.list)){
				si_set_full(&si);
				add_new_valset(addr_valset, 
					GLOBAL_REGION, 0, si);
			} else {
				valset_mul_offset(&tmp,
					&CAST2_USE(index->node)->valset,
					opd->data.expression.scale);

				valset_add_offset(addr_valset,
					&tmp, opd->data.expression.disp);

				list_for_each_entry(entry, &addr_valset->list, list) {
					valset_modify_region(entry, GLOBAL_REGION, 0);
				}
			}
		} else {
			if (!list_empty(&CAST2_USE(base->node)->valset.list) &&
			    list_empty(&CAST2_USE(index->node)->valset.list)){
				si_set_full(&si);
				region = get_region(&CAST2_USE(base->node)->valset);
				add_new_valset(addr_valset, region->type, region->sub_id, si);
			} else {
				valset_mul_offset(&tmp,
					&CAST2_USE(index->node)->valset,
					opd->data.expression.scale);

				valset_add_offset(&tmp, &tmp, opd->data.expression.disp);

				valset_add(addr_valset, &CAST2_USE(base->node)->valset, &tmp);
			}
		}
	}

	/*
	  A memory operand is untrackable if the size of all the a-locs
	  accessed by the memory operand is greater than 4 bytes,
	  or if the value-set obtained by evaluating the address expression
	  of memory operand is T(vs).
	  A memory operand is weakly-trackable if the size of some a-loc
	  accessed by the memory operand is less than or equal to 4 bytes,
	  and the value-set obtained by evaluating the address expression
	  of memory operand is not T(vs).
	  A memory operand is strongly-trackable if the size of all the a-locs
	  accessed by the memory operand is less than or equal to 4 bytes,
	  and the value-set obtained by evaluating the address expression
	  of the memory is not T(vs).
	 */ 
	if (translate_datatype_to_byte(opd->datatype) > ADDR_SIZE_IN_BYTE) {
		return;
	}

	if (exp->node_type == DefNode) {
		//infer before value set of memory access
		prevdef = vs_find_prev_def_of_def(exp, &type);
		if ((prevdef) && ((type == EXACT) || (type == SUPER))) {
			assign_def_before_value_set(exp,
				&CAST2_DEF(prevdef->node)->aft_valset);
		}
	}

	if (exp->node_type == UseNode) {
		// infer value set of memory access
		prevdef = vs_find_prev_def_of_use(exp, &type);
		if ((prevdef) && ((type == EXACT) || (type == SUPER))) {
			assign_use_value_set(exp,
				&CAST2_DEF(prevdef->node)->aft_valset);
		}
	}
}

// not only res_expression, but also resolve base,index registers
void vs_traverse_inst_operand(re_list_t *inst, re_list_t **use, re_list_t **def, int *nuse, int *ndef) {
	re_list_t *entry;

	*nuse = 0;
	*ndef = 0;

	list_for_each_entry_reverse(entry, &inst->list, list){
		if(entry == &re_ds.head) break;
		if(entry->node_type == InstNode) break;

		if(entry->node_type == UseNode){
			if(node_is_exp(entry, true)){
				//LOG(stdout, "resolve expression:\n");
				//print_node(entry);
				vs_res_expression(entry);
			} else if(CAST2_USE(entry->node)->usetype == Opd) {
				//LOG(stdout, "resolve other operands:\n");
				//print_node(entry);
				vs_res_non_expression(entry);
			}

			if(CAST2_USE(entry->node)->usetype == Opd) {
				use[(*nuse)++] = entry;
			}
		}

		if(entry->node_type == DefNode){
			if(node_is_exp(entry, false)){
				//LOG(stdout, "resolve expression:\n");
				//print_node(entry);
				vs_res_expression(entry);
			} else {
				//LOG(stdout, "resolve other operands:\n");
				//print_node(entry);
				vs_res_non_expression(entry);
			}
			def[(*ndef)++] = entry;
		}
	}
}

void assign_esp_in_function(re_list_t *func_start) {
	x86_op_t *opd;
	re_si si;
	re_list_t *entry, *instnode = func_start;
	int funcid = CAST2_INST(func_start->node)->funcid;
	list_for_each_entry(entry, &func_start->list, list) {
		print_node(entry);
		if (entry == &re_ds.head) break;

		if (entry->node_type == InstNode) {
			instnode = entry;
			continue;
		}

		if (CAST2_INST(instnode->node)->funcid != funcid)
			continue;

		if (entry->node_type == DefNode) {
			opd = CAST2_DEF(entry->node)->operand;
			if (x86_opd_is_esp(opd)) {
				// set esp before value
				INIT_SI(si, 0, 0, 0);
				add_new_valset(&(CAST2_DEF(entry->node)->bef_valset), STACK_REGION, funcid, si);
				break;
			}
		}

		if (entry->node_type == UseNode) {
			opd = CAST2_USE(entry->node)->operand;
			if (((CAST2_USE(entry->node)->usetype == Base)&&
			     (x86_base_is_esp(opd))) ||
			    ((CAST2_USE(entry->node)->usetype == Index)&&
			     (x86_index_is_esp(opd))) ||
			    ((CAST2_USE(entry->node)->usetype == Opd) &&
			     (x86_opd_is_esp(opd)))) {
				INIT_SI(si, 0, 0, 0);
				add_new_valset(&(CAST2_USE(entry->node)->valset), STACK_REGION, funcid, si);
			}
		}
	}
}

void set_segment_for_region() {
	
	re_list_t *entry;
	re_region_t *region;
	int seg_id;
	unsigned long base_addr;
	re_value_set *vs_entry;

	// first set gs region base address
	region = find_region(GS_REGION, 0);
	seg_id = get_segment_from_addr(re_ds.coredata->corereg.gs_base);
	assert(seg_id != -1);
	set_segment_id(region, seg_id);
	set_base_address(region, re_ds.coredata->corereg.gs_base);

	list_for_each_entry(entry, &re_ds.head.list, list) {
		if (entry->node_type == InstNode) continue;

		print_node(entry);
		if (entry->node_type == DefNode) {
			// [eax] : if address == 0 => address is known 
			// eax : address == 0
			if ((CAST2_DEF(entry->node)->address) &&
			    (!list_empty(&CAST2_DEF(entry->node)->addr_valset.list))) {
				LOG(stdout, "Define Address\n");
				seg_id = get_segment_from_addr(CAST2_DEF(entry->node)->address);
				region = get_region(&CAST2_DEF(entry->node)->addr_valset);
				set_segment_id(region, seg_id);

				vs_entry = find_region_in_valset(&CAST2_DEF(entry->node)->addr_valset, region);
				if (si_is_one_value(&vs_entry->si)) {
					set_base_address(region,
						CAST2_DEF(entry->node)->address - 
						vs_entry->si.lower_bound);
				}
			}

			if ((CAST2_DEF(entry->node)->val_stat & BeforeKnown) &&
			    (!list_empty(&CAST2_DEF(entry->node)->bef_valset.list))) {
				LOG(stdout, "Define Before Value\n");
				seg_id = get_segment_from_addr(CAST2_DEF(entry->node)->beforeval.dword);
				region = get_region(&CAST2_DEF(entry->node)->bef_valset);
				set_segment_id(region, seg_id);

				vs_entry = find_region_in_valset(&CAST2_DEF(entry->node)->bef_valset, region);
				if (si_is_one_value(&vs_entry->si)) {
					set_base_address(region,
						CAST2_DEF(entry->node)->beforeval.dword - 
						vs_entry->si.lower_bound);
				}
			}
			if ((CAST2_DEF(entry->node)->val_stat & AfterKnown) &&
			    (!list_empty(&CAST2_DEF(entry->node)->aft_valset.list))) {
				LOG(stdout, "Define After Value\n");
				seg_id = get_segment_from_addr(CAST2_DEF(entry->node)->afterval.dword);
				region = get_region(&CAST2_DEF(entry->node)->aft_valset);
				set_segment_id(region, seg_id);

				vs_entry = find_region_in_valset(&CAST2_DEF(entry->node)->aft_valset, region);
				if (si_is_one_value(&vs_entry->si)) {
					set_base_address(region,
						CAST2_DEF(entry->node)->afterval.dword - 
						vs_entry->si.lower_bound);
				}
			}
		}

		if (entry->node_type == UseNode) {
			// [eax+esi] : if address == 0 => address is known 
			// eax : address == 0
			// esi : address == 0
			if ((CAST2_USE(entry->node)->address) &&
				(!list_empty(&CAST2_USE(entry->node)->addr_valset.list))) {
				LOG(stdout, "Use Address\n");
				seg_id = get_segment_from_addr(CAST2_USE(entry->node)->address);
				region = get_region(&CAST2_USE(entry->node)->addr_valset);
				set_segment_id(region, seg_id);

				vs_entry = find_region_in_valset(&CAST2_USE(entry->node)->addr_valset, region);
				if (si_is_one_value(&vs_entry->si)) {
					set_base_address(region,
						CAST2_USE(entry->node)->address -
						vs_entry->si.lower_bound);
				}
			}

			if ((CAST2_USE(entry->node)->val_known) &&
				(!list_empty(&CAST2_USE(entry->node)->valset.list))) {
				LOG(stdout, "Use Value\n");
				seg_id = get_segment_from_addr(CAST2_USE(entry->node)->val.dword);
				region = get_region(&CAST2_USE(entry->node)->valset);
				set_segment_id(region, seg_id);

				vs_entry = find_region_in_valset(&CAST2_USE(entry->node)->valset, region);
				if (si_is_one_value(&vs_entry->si)) {
					set_base_address(region,
						CAST2_USE(entry->node)->val.dword -
						vs_entry->si.lower_bound);
				}
			}
		}
	}
}

// Infer Region and Offset information from address
void set_valset_from_address(re_value_set *head, unsigned long address) {
	assert(valset_is_empty(head));
	LOG(stdout, "======Infer Value Set from Address======\n");
	// need to design region in detail so that
	// we can verify one concrete address is in which region

	int seg_id;
	// record the num of region that includes that address
	// Special Region: Heap Region
	re_si si;
	re_region_t *region = NULL, *region_entry;

	seg_id = get_segment_from_addr(address);

	LOG(stdout, "Address is %lx\t Segment Id is %d\n", address, seg_id);

	if (seg_id == -1) {
		LOG(stdout, "warning: no corresponding segment\n");
		// set region to constant region when seg_id == -1
		INIT_SI(si, 0, address, address);
		add_new_valset(head, CONST_REGION, 0, si);
		//print_valset(head);
		return;
	}

	size_t region_num = 0;
	list_for_each_entry(region_entry, &re_ds.region_head.list, list) {
		if (seg_id == region_entry->segment_id) {
			region_num++;
			region = region_entry;
			//print_one_region(region);
		}
	}

	if (region_num > 1) {
		LOG(stdout, "warning: one address in two regions. Interesting\n");
		return;
	}

	if (region_num == 0) {
		// Check whether this address is in the Global region of library
		LOG(stdout, "Check whether one address is in the Global Region of Library\n");
		list_for_each_entry(region_entry, &re_ds.region_head.list, list) {
			if (region_entry->type == GLOBAL_REGION && region_entry->sub_id != 0) {
				if (seg_id == region_entry->segment_id - 1) {
					region = region_entry;
					break;
				}
			}
		}
		if (!region) {
			LOG(stdout, "warning: add new region here\n");
			// add one new region
			region = select_newest_region(OTHER_REGION);
			if (!region) {
				region = add_new_region(OTHER_REGION, 0);
			} else {
				region = add_new_region(OTHER_REGION, region->sub_id + 1);
			}

			set_segment_id(region, seg_id);
			set_base_address(region, re_ds.coredata->coremem[seg_id].low);
		}
	}

	if (region->base_addr != 0) {
		INIT_SI(si, 0, address - region->base_addr, address - region->base_addr);
	} else {
		si_set_full(&si);
	}

	//FIXME: add new api to directly use region pointer
	add_new_valset(head, region->type, region->sub_id, si);
	//print_valset(head);
}

// After the first round of value set analysis,
// always do this round of VSA
void one_round_of_vsa(){
	unsigned index; 
	re_list_t *entry;

	list_for_each_entry(entry, &re_ds.head.list, list){

		if (entry->node_type != InstNode) continue;

		//if (vs_inst_complete(entry)) continue;
	
		index = CAST2_INST(entry->node)->inst_index;

		int handler_index = insttype_to_index(re_ds.instlist[index].type);

		if (handler_index >= 0) {
			vs_handler[handler_index](entry);
		}

		print_info_of_current_inst(entry);
	}

	// if valset_inst_correctness is enabled, just comment this check
	valset_correctness_check();
}
#endif

bool truth_verify_noalias(re_list_t *exp, re_list_t *entry) {
	unsigned long addr_entry, addr_exp;
	size_t size_entry, size_exp;
	x86_op_t *opd;

	opd = GET_OPERAND(exp);
	addr_exp = GET_TRUE_ADDR(exp);
	size_exp = translate_datatype_to_byte(opd->datatype);

	opd = GET_OPERAND(entry);
	addr_entry = GET_TRUE_ADDR(entry);
	size_entry = translate_datatype_to_byte(opd->datatype);
	//LOG(stdout, "addr_exp 0x%lx, size_exp %d, addr_entry 0x%lx, size_entry %d\n",
	//	    addr_exp, size_exp, addr_entry, size_entry);

	return nooverlap_mem(addr_exp, size_exp, addr_entry, size_entry);
}

static bool is_effective_reg(re_list_t *entry) {
	bool result = false;	
	x86_op_t *opd;

	if ((entry->node_type == DefNode) ||
	    (entry->node_type == UseNode && 
	     CAST2_USE(entry->node)->usetype == Opd)) {
		opd = GET_OPERAND(entry);

		if (opd->type == op_register) {
			result = true;
		}
	}
	if (entry->node_type == UseNode &&
	    CAST2_USE(entry->node)->usetype != Opd) {
			result = true;
	}
	return result;

}

static bool is_effective_exp(re_list_t *entry) {
	bool result = false;	
	x86_op_t *opd;

	if ((entry->node_type == DefNode) ||
	    (entry->node_type == UseNode && CAST2_USE(entry->node)->usetype == Opd)) {
		opd = GET_OPERAND(entry);

		if (opd->type == op_expression) {
			result = true;
		}
	}
	return result;
}

// only used to get value of register
static unsigned long get_regvalue_from_pin(re_list_t *reg) {
	unsigned long value = 0x0;
	operand_val_t *regvals;
	re_list_t *instnode;
	unsigned int index, regindex, regnum = 0;

	instnode = find_inst_of_node(reg);
	index = CAST2_INST(instnode->node)->inst_index;
	
	regvals = &re_ds.oplog_list.opval_list[index];

	assert(is_effective_reg(reg));

	switch (reg->node_type) {
	case DefNode:
		regnum = CAST2_DEF(reg->node)->operand->data.reg.id;
		break;
	case UseNode:
		if (CAST2_USE(reg->node)->usetype == Index) {
			regnum = CAST2_USE(reg->node)->operand->data.expression.index.id;
		} else if (CAST2_USE(reg->node)->usetype == Base) {
			regnum = CAST2_USE(reg->node)->operand->data.expression.base.id;
		} else if (CAST2_USE(reg->node)->usetype == Opd) {
			regnum = CAST2_USE(reg->node)->operand->data.reg.id;
		} else {
			assert(0 && "Error Invocation");
		}
		break;
	}

	assert(regnum);

	for(regindex = 0; regindex < regvals->regnum; regindex++){
		if (regvals->regs[regindex].reg_num == regnum) {
			value = regvals->regs[regindex].val.dword;
			break;
		}
	}
	return value;
} 

static unsigned long truth_address(re_list_t *exp) {
	unsigned long address = 0x0;
	unsigned long base_addr, index_addr;
	re_list_t *instnode, *definst, *defnode;
	int inst_index, type;
	re_list_t *base, *index;
	x86_op_t *opd;

	// get all the registers and calculate them together
	base_addr = 0x0;
	index_addr = 0x0;
	
	opd = GET_OPERAND(exp);

	get_element_of_exp(exp, &index, &base);
	
	// there may exist define of such register
	if (index) {
		instnode = find_inst_of_node(index);
		inst_index = CAST2_INST(instnode->node)->inst_index;
		
		index_addr = get_regvalue_from_pin(index);

		defnode = find_prev_def_of_use(index, &type);
		if (defnode) {
			definst = find_inst_of_node(defnode);
			//print_node(instnode);
			//print_node(defnode);
			assert(instnode->id != definst->id);
		}
	}

	if (base) {
		instnode = find_inst_of_node(base);
		inst_index = CAST2_INST(instnode->node)->inst_index;

		base_addr = get_regvalue_from_pin(base);

		if (re_ds.instlist[inst_index].type == insn_leave) {
			// esp = ebp
			// directly get address from ebp in intel pin trace
			re_list_t *dst[NOPD], *src[NOPD];
			int nuse, ndef;
			obtain_inst_operand(instnode, src, dst, &nuse, &ndef);
			//print_node(src[1]);
			base_addr = get_regvalue_from_pin(src[1]);
		} else {
			defnode = find_prev_def_of_use(base, &type);
			if (!defnode) goto cal_address;
			definst = find_inst_of_node(defnode);
			//print_node(instnode);
			//print_node(defnode);
			if (instnode->id != definst->id) goto cal_address;
			if (defnode->id > exp->id) {
				if (re_ds.instlist[inst_index].type == insn_push) {
					base_addr -= ADDR_SIZE_IN_BYTE;
				} else if (re_ds.instlist[inst_index].type == insn_call) {
					base_addr -= ADDR_SIZE_IN_BYTE;
				}
			} else {
				// No need to do anything
				// push [esp+0x1C]
			}
		}
	}
	
cal_address:
	address = base_addr + index_addr * opd->data.expression.scale + 
		(int)(opd->data.expression.disp);

	if (op_with_gs_seg(opd)) {
		address += re_ds.coredata->corereg.gs_base;
	}

	//LOG(stdout, "Address of expression is 0x%lx\n", address);

	return address;
}


void calculate_truth_address() {

	re_list_t *entry;
	unsigned int *true_addr;
	list_for_each_entry_reverse(entry, &re_ds.head.list, list) {
		if (entry->node_type == InstNode) continue;
		// only traverse the effective expressions
		if (is_effective_exp(entry)) {
			if (entry->node_type == DefNode) {
				CAST2_DEF(entry->node)->true_addr = truth_address(entry);
			} else {
				CAST2_USE(entry->node)->true_addr = truth_address(entry);
			}
		}
	}
}

void find_access_related_address(unsigned long address) {
	re_list_t *entry;
	unsigned long addr_exp;
	LOG(stdout, "List All the instructions related with %lx\n", address);
	list_for_each_entry(entry, &re_ds.head.list, list) {
		if (entry->node_type == InstNode) continue;
		addr_exp = GET_TRUE_ADDR(entry);
		if (addr_exp == address) {
			print_info_of_current_inst(find_inst_of_node(entry));
		}
	}
}

#include "reverse_exe.h"
#include "bin_alias.h"
#include "insthandler.h"

void fill_func_info(int funcid) {
	re_list_t *entry;
	unsigned int index;
	unsigned long maxvalue = 0, minvalue = ULONG_MAX;
	unsigned long esp_min, esp_max;
	// note architecture
	//LOG(stdout, "max value 0x%x, min value 0x%x\n", maxvalue, minvalue);
	list_for_each_entry(entry, &re_ds.head.list, list){	
		if(entry->node_type != InstNode) continue;
		index = CAST2_INST(entry->node)->inst_index;
		if (CAST2_INST(entry->node)->funcid != funcid) continue;

		// find the first esp and set it as start of stack
		// find the last esp and set it as end of stack
		if(get_esp_value_from_inst(entry, &esp_min, &esp_max)) {
			//print_info_of_current_inst(entry);
			//LOG(stdout, "esp min value 0x%x, esp max value 0x%x\n", esp_min, esp_max);
			//if (esp_min <= esp_max) {
			if (esp_min < minvalue) minvalue = esp_min;
			if (esp_max > maxvalue) maxvalue = esp_max;
			//LOG(stdout, "max value 0x%lx, min value 0x%lx\n", maxvalue, minvalue);
		}
	}
	re_ds.flist[funcid].stack_start = minvalue;
	re_ds.flist[funcid].stack_end = maxvalue;
} 

// find the start, end of stack
void adjust_stack_boundary() {
	int i;
	for (i=0; i<=re_ds.maxfuncid; i++) {
		//LOG(stdout, "adjust function %d\n", i);
		fill_func_info(i);
	}	
}

//adjust the boundary of a function
void adjust_func_boundary(re_list_t* instnode){

	inst_node_t* inst; 
	x86_insn_t *x86inst;	
	unsigned funcid;
	unsigned instid; 

	inst = CAST2_INST(instnode->node);
	instid = inst->inst_index;
	funcid = inst->funcid;
	x86inst = &re_ds.instlist[inst->inst_index];

//Maximal function number is MAXFUNC	
	assert(funcid < MAXFUNC);

//check if this is the first function 
//if so
	//for the first function, the start is always 0
	//so we do not adjust here 
	if(funcid == 0){
		//the end is increasing
		assert(re_ds.flist[funcid].end <= instid);
		re_ds.flist[funcid].end = instid;
	}else{

		if (re_ds.flist[funcid].start == 0)
			re_ds.flist[funcid].start = instid;

		assert(re_ds.flist[funcid].start <= instid);

		re_ds.flist[funcid].end = instid > re_ds.flist[funcid].end ? instid : re_ds.flist[funcid].end;
	}
//else
	if(x86inst->type == insn_return)
		re_ds.flist[funcid].returned = true;	

}

void init_reg_use(re_list_t* usenode, re_list_t* uselist){
/*logics here
check if this is the first use
if so, assign the value from the log
*/
	use_node_t * use; 
	inst_node_t * inst; 
	re_list_t *prevdef; 
	int dtype;	
	operand_val_t *regvals;
	int regid; 
	int regindex;

	use = CAST2_USE(usenode->node);
	inst = CAST2_INST(find_inst_of_node(usenode)->node);

	//this is not a use of register
	if(use->usetype == Opd && use->operand->type == op_expression)
		return;	

	prevdef = find_prev_def_of_def(usenode, &dtype);
	//there is a define before, do nothing
	if(prevdef)
		return; 
	
	if(use->usetype == Opd)
		regid = use->operand->data.reg.id;		
	if(use->usetype == Base)
		regid = use->operand->data.expression.base.id;
	if(use->usetype == Index)
		regid = use->operand->data.expression.index.id;

	regvals = &re_ds.oplog_list.opval_list[inst->inst_index];

	for(regindex = 0; regindex < regvals->regnum; regindex++){
		if (regvals->regs[regindex].reg_num == regid){
			assign_use_value(usenode, regvals->regs[regindex].val);	
			add_to_uselist(usenode, uselist);
			return; 
		}	
	}
}

bool is_function_start(re_list_t *instnode) {
	int index = CAST2_INST(instnode->node)->inst_index;
	int i;
	for (i=0; i<=re_ds.maxfuncid; i++) {
		if (re_ds.flist[i].end == index) {
			return 1;
		}
	}
	return 0;
}

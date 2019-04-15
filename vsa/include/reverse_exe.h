#ifndef __REV_EXE__
#define __REV_EXE__

#include <stdbool.h>
#include <setjmp.h>
#include <limits.h>
#include "elf_core.h"
#include "list.h"
#include "inst_data.h"
#include "re_valueset.h"
#include "common.h"


#define INIT_RE(re_ds, a, b, c, d) \
	re_ds.instnum = a; \
	re_ds.instlist = b; \
	re_ds.coredata = c; \
	re_ds.current_id = 0; \
	re_ds.alias_id = 0; \
	re_ds.rec_count = 0;

#define INSERTED 0
#define EINSERT -1

typedef enum compare_result {
	NO_MATCH = 0,
	SUB = 1,
	SUPER,
	EXACT,
	OVERLAP
} cmp_result;

#define ROUND_LIMIT 1

#define REC_LIMIT 2

#define RE_RES(red, reu, rei) \
	!list_empty(&red->deflist) || \
	!list_empty(&reu->uselist) || \
	!list_empty(&rei->instlist)

#define CAST2_DEF(def) ((def_node_t *)def)
#define CAST2_USE(use) ((use_node_t *)use)
#define CAST2_INST(inst) ((inst_node_t *)inst)

#define NOPD 16

#define MAXFUNC 10000

typedef union valset_struct{
	unsigned char byte; 	 /* 1-byte */
	unsigned short word; 	 /* 2-byte */
	unsigned long dword; 	 /* 4-byte */
	unsigned long qword[2];	 /* 8-byte */
	unsigned long dqword[4]; /* 16-byte*/
}valset_u; 


#define MAX_REG_IN_INST 0x6

typedef struct opv{
	int reg_num; 
	valset_u val; 
}opv_t;

typedef struct operand_val{
	size_t regnum; 
	opv_t regs[MAX_REG_IN_INST];
}operand_val_t; 

typedef struct opval_list{
	int log_num; 
	operand_val_t *opval_list;
}opval_list_t; 

typedef struct dlregion_list {
        size_t dlreg_num;
        int dlreg_list[2];
}dlregion_list_t;

enum nodetype{
	InstNode = 0x01,
	DefNode,
	UseNode
};

enum defstatus{
	Unknown = 0x00,
	BeforeKnown = 0x01,
	AfterKnown = 0x02,
	BothKnown = 0x03
};

enum u_type{
	Opd = 0,
	Base,
	Index
};

typedef struct func_info_struct{
	bool returned; 
	unsigned start;
	unsigned end; 

	unsigned stack_start;
	unsigned stack_end;
}func_info_t; 

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)

typedef enum malloc_family{
	NONE,
	MALLOC,
	FREE,
	CALLOC,
	REALLOC
}mal_family_t;

// void *malloc(size_t size);
// void free(void *ptr);
// void *calloc(size_t nmemb, size_t size);
// void *realloc(void *ptr, size_t size);
#define MALLOC_FUNC_NUM 5

// only just for libc.so and ld.so
#define GET_PC_NUM 5

typedef struct valset_heu {
	unsigned long malloc_addrs[MALLOC_FUNC_NUM];
	int pic_count;
	unsigned long get_pc_func[GET_PC_NUM];
}valset_heu_t;

typedef enum valset_heu_type {
	HEU_MALLOC = 0,
	HEU_GLOBAL
} heu_type;

#define GET_ADDR_VALSET(entry) \
	((entry)->node_type == DefNode) ? \
	(&CAST2_DEF((entry)->node)->addr_valset) : \
	(&CAST2_USE((entry)->node)->addr_valset)

#endif

#define GET_OPERAND(entry) \
	((entry)->node_type == DefNode) ? \
	(CAST2_DEF((entry)->node)->operand) : \
	(CAST2_USE((entry)->node)->operand)

#define GET_TRUE_ADDR(entry) \
	((entry)->node_type == DefNode) ? \
	(CAST2_DEF((entry)->node)->true_addr) : \
	(CAST2_USE((entry)->node)->true_addr)

#define GET_DL_REGION(entry) \
	((entry)->node_type == DefNode) ? \
	(CAST2_DEF((entry)->node)->dl_region) : \
	(CAST2_USE((entry)->node)->dl_region)

//a key data struct for define node
//status: showing the value status
typedef struct def_node_struct{
	x86_op_t* operand; 			
	enum defstatus val_stat;

	valset_u beforeval; 
	valset_u afterval;

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	// in order to maintain point-to set
	re_value_set bef_valset;
	re_value_set aft_valset;
#endif

	region_type dl_region;

	// means unknown when 0
	unsigned address;

	// assigned by calculate_truth_address
	// true address obtained from intel pin
	unsigned true_addr;

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	// value set obtain from value set analysis or DL
	re_value_set addr_valset;
#endif
}def_node_t;


typedef struct use_node_struct{
	enum u_type usetype;
	x86_op_t* operand;

	bool val_known; 		
	valset_u val; 

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	// in order to maintain point-to set
	re_value_set valset;
#endif

	region_type dl_region;

	// means unknown when 0
	unsigned address;

	// assigned by calculate_truth_address
	// true address obtained from intel pin
	unsigned true_addr;

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	// value set obtain from value set analysis or DL
	re_value_set addr_valset;
#endif
}use_node_t; 


typedef struct inst_node_struct{
	unsigned inst_index; 
	corereg_t corereg; 
	unsigned funcid; 
}inst_node_t;    

//data struct for node in re_list
//node_type: type of a node
//node: point to the contents of the node
//list: double linked list


typedef struct re_list_struct{
	unsigned id;

	enum nodetype node_type;
	void* node; 

//core linked list
	struct list_head list;

//for value resolving
	struct list_head deflist;
	struct list_head uselist;
	struct list_head instlist; 

// memory operand list
        struct list_head memlist;

//for alias
	struct list_head umemlist; 

}re_list_t;

typedef enum alias_module_enum {
	NO_ALIAS_MODULE = 0,
	HYP_TEST_MODULE,
	VALSET_MODULE,
	VALSET_DL_MODULE,
	GR_TRUTH_MODULE
} alias_module_t;

//main data structure for reverse execution 
typedef struct re_struct{
	//which instruction id is being processed
	unsigned current_id;
	unsigned alias_id;

	//number of instruction in total
	size_t instnum;

	//the list of instructions
	x86_insn_t * instlist; 
	//the data loaded from core dump
	coredata_t * coredata; 

	//head of the core list
	re_list_t head; 

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	// region list head
	re_region_t region_head;
	size_t region_count;
#endif

	//trace location to return when conflict detected during alias verification
	jmp_buf aliasret;

	//are these two really needed in this version?
	re_list_t aliashead;
	int alias_offset; 	

	//track the number of layer the alias verification is in
	int rec_count;
	//track if alias verification is enabled
	bool resolving;	
	//track if ground truth verification is enabled
	alias_module_t alias_module;

	//the operand that leads to the crash, as the starting point for taint analysis 
	x86_op_t* root;

	//the list about log
	opval_list_t oplog_list; 

	//region list
	dlregion_list_t *dlregionlist;

	// NOTE: don't use it before init_vsa_analysis
	int maxfuncid;

	func_info_t flist[MAXFUNC];

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	// the value of first esp
	unsigned long esp_value;
	// address of malloc family functions
	valset_heu_t valset_heu;
#endif

	unsigned curinstid; 
}re_t;


extern re_t re_ds;

unsigned long reverse_instructions();

unsigned maxfuncid(void);

unsigned long load_log(char* log_path, operand_val_t *oploglist);

re_list_t * add_new_inst(unsigned index);

int load_dlregion(char *dlregion_file, dlregion_list_t *dlregionlist);

re_list_t * add_new_define(x86_op_t * opd);

re_list_t * add_new_use(x86_op_t * opd, enum u_type type);

void assign_def_before_value(re_list_t * def, valset_u val);

void assign_def_after_value(re_list_t * def, valset_u val);

void re_resolve(re_list_t *re_uselist, re_list_t *re_deflist, re_list_t *re_instlist);

void resolve_use(re_list_t *re_uselist, re_list_t *re_deflist, re_list_t *re_instlist);

void resolve_define(re_list_t *re_uselist, re_list_t *re_deflist, re_list_t *re_instlist);

void resolve_inst(re_list_t *re_uselist, re_list_t *re_deflist, re_list_t *re_instlist);

int compare_two_targets(re_list_t* first, re_list_t * second);

bool ok_to_get_value(re_list_t *entry);

re_list_t * find_prev_use_of_use(re_list_t* use, int *type);

re_list_t * find_next_use_of_use(re_list_t* use, int *type);

re_list_t * find_prev_def_of_use(re_list_t* use, int *type);

re_list_t * find_next_def_of_use(re_list_t* use, int *type);

re_list_t * find_prev_def_of_def(re_list_t* def, int *type);

re_list_t * find_next_def_of_def(re_list_t* def, int *type);

re_list_t * find_inst_of_node(re_list_t *node);

size_t size_of_node(re_list_t* node);

// According to node_type, 
// check if this node in the corresponding list
bool check_node_in_list(re_list_t *node, re_list_t *list);

int insttype_to_index(enum x86_insn_type type);

int syscall_to_index(unsigned long sys_id);

int check_inst_resolution(re_list_t* inst);

bool node_is_exp(re_list_t* node, bool use);

bool address_is_known(re_list_t *node);

void res_expression(re_list_t * exp, re_list_t *uselist);

bool get_parameter_of_sysenter(re_list_t *instnode, unsigned long address, enum x86_op_datatype datatype, valset_u *rv);

void extend_corelist_from_sysenter(re_list_t *instnode, unsigned long address, enum x86_op_datatype datatype, unsigned long size);

void assign_use_value(re_list_t *use, valset_u val);

void obtain_inst_operand(re_list_t* inst, re_list_t **use, re_list_t **def, int *nuse, int *ndef);

void obtain_inst_elements(re_list_t* inst, re_list_t **use, re_list_t **def, int *nuse, int *ndef);

void traverse_inst_operand(re_list_t* inst, re_list_t **use, re_list_t **def, re_list_t* uselist, re_list_t* deflist, int *ndef, int *nuse);

void obtain_inst_operand(re_list_t* inst, re_list_t **use, re_list_t **def, int *nuse, int *ndef);

void split_expression_to_use(x86_op_t* opd);

bool check_next_unknown_write(re_list_t *listhead, re_list_t *def, re_list_t *target);

bool resolve_alias(re_list_t* exp, re_list_t* uselist);

//void add_to_corelist(re_list_t *tmp_node, re_list_t *listhead);

void add_to_umemlist(re_list_t * exp);

void add_to_deflist(re_list_t *entry, re_list_t *listhead);

void add_to_uselist(re_list_t *entry, re_list_t *listhead);

void add_to_instlist(re_list_t *entry, re_list_t *listhead);

void add_to_instlist_tail(re_list_t *entry, re_list_t *listhead);

void remove_from_umemlist(re_list_t* exp);

void remove_from_deflist(re_list_t *entry, re_list_t *listhead);

void remove_from_uselist(re_list_t *entry, re_list_t *listhead);

void remove_from_instlist(re_list_t *entry, re_list_t *listhead);

//void add_to_corelist(re_list_t *tmp_node);

void destroy_corelist();

void destroy_dlregionlist(dlregion_list_t *dlregionlist);

void fork_corelist(re_list_t *newhead, re_list_t *oldhead);

void delete_corelist(re_list_t *head);

bool node1_add_before_node2(re_list_t *node1, re_list_t* node2);

re_list_t* get_new_exp_copy(re_list_t* exp);

void fork_umemlist();

void get_element_of_exp(re_list_t* exp, re_list_t ** index, re_list_t ** base);

bool unknown_expression(re_list_t * exp);

bool assign_mem_val(re_list_t* exp, valset_u * rv, re_list_t * use);

void zero_valset(valset_u *vt);

void one_valset(valset_u *vt);

//void add_dq_valset(valset_u *vt1, valset_u *vt2, valset_u *vt3);

void clean_valset(valset_u *vt, enum x86_op_datatype datatype, bool sign);

void sign_extend_valset(valset_u *vt, enum x86_op_datatype datatype);

bool sign_of_valset(valset_u *vt, enum x86_op_datatype datatype);

void get_src_of_def(re_list_t* def, re_list_t **use, int *nuse);

void resolve_heuristics(re_list_t* instnode, re_list_t *re_deflist, re_list_t *re_uselist, re_list_t *re_instlist);

bool obstacle_between_two_targets(re_list_t *listhead, re_list_t* entry, re_list_t *target);

void correctness_check(re_list_t * instnode);

re_list_t *get_entry_by_id(unsigned id);

re_list_t *get_entry_by_inst_id(unsigned inst_index);

re_list_t *find_first_jcc(re_list_t *instnode);

bool get_esp_value_from_inst(re_list_t *inst, unsigned long *esp_before, unsigned long *esp_after);

#ifdef FIX_OPTM
void fix_optimization(re_list_t* inst);
#endif

bool infer_valset_from_entry(re_list_t *entry);

unsigned long infer_valset_from_re();

void one_round_of_re();

void re_statistics();

void calculate_truth_address();

void find_access_related_address(unsigned long address);

bool truth_verify_noalias(re_list_t *exp, re_list_t *entry);

bool ht_verify_noalias(re_list_t *exp, re_list_t *entry);

void load_region_from_DL();
bool dl_region_verify_noalias(re_list_t *exp1, re_list_t *exp2);
void ratio_noalias_pair_ht();

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)

/*
 * related to value set manipulation
 */
void init_first_esp();

unsigned long find_value_of_first_esp();

void load_valset_heuristic();

void init_region_info();

void init_vsa_analysis();

void valset_statistics();

void sign_extend_value_set(re_value_set *head, enum x86_op_datatype datatype);

void vs_traverse_inst_operand(re_list_t *inst, re_list_t **use, re_list_t **def, int *nuse, int *ndef);

void vs_res_expression(re_list_t *exp);

void vs_res_non_expression(re_list_t *entry);

void vs_res_reg_in_exp(re_list_t *entry);

re_list_t * vs_find_prev_use_of_use(re_list_t *use, cmp_result *type);

re_list_t * vs_find_prev_def_of_use(re_list_t *use, cmp_result *type);

re_list_t * vs_find_prev_use_of_def(re_list_t *use, cmp_result *type);

re_list_t * vs_find_prev_def_of_def(re_list_t *use, cmp_result *type);

void assign_use_value_set(re_list_t *use, re_value_set *valset);

void assign_def_before_value_set(re_list_t *def, re_value_set *valset);

void assign_def_after_value_set(re_list_t *def, re_value_set *valset);

re_list_t * find_return_value(re_list_t *ret_inst);

bool valset_verify_noalias(re_list_t *entry, re_list_t *target);

void ratio_noalias_pair_vsa();

void valset_correctness_check();

void set_segment_for_region();

void set_valset_from_address(re_value_set *head, unsigned long address);

void one_round_of_vsa();
#endif

#endif

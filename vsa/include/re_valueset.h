#ifndef VS_VALUESET_H
#define VS_VALUESET_H

#include <elf.h>
#include <stdbool.h>
#include "list.h"
#include "common.h"

typedef enum region_type_enum {
	REG_REGION = 0, // only one
	CONST_REGION,	// only one
	GLOBAL_REGION,	// one for each external library
	//RO_REGION, 	// one for each external library
	HEAP_REGION,	// only one, but separated by malloc sites
	STACK_REGION,	// one for each thread
	GS_REGION,	// one for each thread
	OTHER_REGION
} region_type;

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)

#define INIT_SI(si, st, low, upper) \
	(si).bits = ADDRESS_SIZE; \
	(si).stride = st; \
	(si).lower_bound = low; \
	(si).upper_bound = upper;

#define IS_IN_SI(value, si) \
	( \
	 (((si)->stride == 0) && \
	  ((value) == (si)->lower_bound)) \
	 || \
	 (((si)->stride) && \
	  (((value)-(si)->lower_bound)%((si)->stride) == 0)) \
	)

// aloc->(memory-region->SI)
// aloc is represented by def/use node

// presentation: s[l, u]
// s: stride
// l: low bound
// u: upper bound
typedef struct strided_interval{
	size_t bits;
	unsigned long stride;
	long lower_bound;
	long upper_bound;
}re_si;

typedef struct region_node{
	int reg_id;
	region_type type;
	int sub_id;
	// Gloal : 0
	// Stack : value of first esp
	// Heap : eax of malloc/calloc/realloc
	Elf32_Addr base_addr;
	//unsigned int size;
	int segment_id;
	char *note;
	struct list_head list; 
}re_region_t;

// value set : memory_region -> SI
typedef struct value_set_node{
	re_region_t *region;
	re_si si;
	struct list_head list;
} re_value_set;

// this data structure is merged into def/use node in corelist
typedef struct abstract_location{
	re_region_t *region;
	long offset;
	// maybe this size can be obtained by node in the corelist
	size_t size;
	struct list_head list;
}re_aloc;

typedef enum {
 	/*
	 * XXXX(N : negative; P : positive)
	 * XXXX = NNNN
	 * 1st X = N : src1->lower_bound < 0 
	 * 2nt X = N : src1->upper_bound < 0 
	 * 3rd X = N : src2->lower_bound < 0 
	 * 4th X = N : src2->upper_bound < 0 
	 */
	NNNN=0,
	NNNP,
	NNPP,
	NPNN,
	NPNP,
	NPPP,
	PPNN,
	PPNP,
	PPPP
}bound_info;


re_region_t * add_new_region(region_type type, int sub_id);

re_region_t *find_region(region_type type, int sub_id);

void set_segment_id(re_region_t *region, int seg_id);

void set_base_address(re_region_t *region, unsigned long address);

re_value_set *find_region_in_valset(re_value_set *valset, re_region_t *region);

re_region_t *select_newest_region(region_type type);

void delete_region_list(re_region_t *head);

bool check_vs_in_list(re_value_set *element, re_value_set *head);

re_value_set * add_new_valset(re_value_set *head, region_type type, int sub_id, re_si si);

bool fork_value_set(re_value_set *newhead, re_value_set *oldhead);

bool delete_value_set(re_value_set *head);

int max_si();

int min_si();

bool si_is_one_value(re_si *si);

bool si_is_full(re_si *si);

void si_set_full(re_si *si);

bool si_contain(re_si *si1, re_si *si2);

bool si_contain_with_len(re_si *si1, int size1, re_si *si2, int size2);

bool si_overlap_with_len(re_si *si1, int size1, re_si *si2, int size2);

bool si_subset(re_si *si1, re_si *si2);

re_si * si_intersection(re_si *si1, re_si *si2);

re_si * si_union(re_si *si1, re_si *si2);

void si_add_offset(re_si *si, int offset);

// take care about dest and src1, they may be alias
void si_add(re_si *dest, re_si *src1, re_si *src2);

void si_sub_offset(re_si *si, int offset);

void si_sub(re_si *dest, re_si *src1, re_si *src2);

void si_mul_offset(re_si *si, int offset);

void si_shl(re_si *si, re_si *src1, re_si *src2);

void si_shr(re_si *si, re_si *src1, re_si *src2);

void si_and_offset(re_si *si, int offset);

void si_and(re_si *dest, re_si *src1, re_si *src2);

void si_xor(re_si *dest, re_si *src1, re_si *src2);

void si_xor_offset(re_si *si, int offset);

void si_or(re_si *dest, re_si *src1, re_si *src2);

void si_not(re_si *si);

void si_neg(re_si *si);

void valset_add_offset(re_value_set *dest, re_value_set *src, int offset);

void valset_add_si(re_value_set *dest, re_value_set *src, re_si *si);

void valset_add(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_sub_offset(re_value_set *dest, re_value_set *src, int offset);

void valset_sub_si(re_value_set *dest, re_value_set *src1, re_si *si);

void valset_sub(re_value_set *dest_aft, re_value_set *dest_bef, re_value_set *src);

void valset_mul_offset(re_value_set *dest, re_value_set *src, int offset);

void valset_mul_one(re_value_set *upper_proc, re_value_set *lower_proc, re_value_set *src1, re_value_set *src2);

void valset_mul_two_or_more(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_div(re_value_set *quotient, re_value_set *reminder, re_value_set *src1,
		re_value_set *src2, re_value_set *src3);

void valset_shl(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_shr(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_rol(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_ror(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_not(re_value_set *dest, re_value_set *src);

void valset_or(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_xor(re_value_set *dest, re_value_set *src1, re_value_set *src2);

void valset_neg(re_value_set *valset);

void valset_and_offset(re_value_set *dest, re_value_set *src, int offset);

void valset_and_si(re_value_set *dest, re_value_set *src, re_si *si);

void valset_and(re_value_set *dest_aft, re_value_set *dest_bef, re_value_set *src);

void valset_set_full(re_value_set *valset);

bool valset_is_empty(re_value_set *dest);

bool valset_is_const(re_value_set *dest);

bool valset_contain(re_value_set *valset1, re_value_set *valset2);

int count_region_num(re_value_set *head);

re_region_t * get_region(re_value_set *head);

re_value_set * valset_intersection(re_value_set *valset1, re_value_set *valset2);

re_value_set * valset_union(re_value_set *valset1, re_value_set *valset2);

void valset_rm_lowerbound(re_value_set *valset);

void valset_rm_upperbound(re_value_set *valset);

void valset_modify_region(re_value_set *valset, region_type type, int sub_id);

bool valset_region_contain(re_value_set *valset1, re_value_set *valset2);

bool valset_region_subset(re_value_set *valset1, re_value_set *valset2);

bool valset_check_same_region(re_value_set *valset1, re_value_set *valset2);

bool mem_valset_exact(re_value_set *valset1, int size1, re_value_set *valset2, int size2);

bool mem_valset_subst(re_value_set *valset1, int size1, re_value_set *valset2, int size2);

bool mem_valset_super(re_value_set *valset1, int size1, re_value_set *valset2, int size2);

bool mem_valset_overlap(re_value_set *valset1, int size1, re_value_set *valset2, int size2);
#endif

#endif

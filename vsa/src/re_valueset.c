#include <limits.h>
#include <math.h>

#include "re_valueset.h"
#include "reverse_exe.h"
#include "reverse_log.h"
#include "insthandler.h"
#include "common.h"

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)

/*
 * API for regions
 */

re_region_t * add_new_region(region_type type, int sub_id) {
	re_region_t *new_region;
	new_region = (re_region_t *) malloc(sizeof(re_region_t));
	if (!new_region) {
		LOG(stderr, "Region malloc error\n");
		return NULL;
	}

	memset(new_region, 0, sizeof(re_region_t));

	new_region->reg_id = re_ds.region_count;
	re_ds.region_count++;

	new_region->type = type;
	new_region->sub_id = sub_id;

	new_region->segment_id = -1;

	list_add_tail(&new_region->list, &re_ds.region_head.list);

	return new_region;
}

// find region from global region list, it cannot be null
re_region_t *find_region(region_type type, int sub_id) {
	re_region_t *result = NULL;
	re_region_t *entry;
	list_for_each_entry(entry, &re_ds.region_head.list, list) {
		if (entry->type == type && entry->sub_id ==sub_id) {
			result = entry;
			break;
		}
	}
	assert(result);
	return result;
}

void set_segment_id(re_region_t *region, int seg_id) {
	if (seg_id == -1) return;

	if (region->segment_id != -1) {
		//assert(region->segment_id == seg_id);
		return;
	}

	region->segment_id = seg_id;
}

void set_base_address(re_region_t *region, unsigned long address) {
	//just skip CONSTANT REGION
	if (region->type == CONST_REGION) return;

	if (get_segment_from_addr(address) != region->segment_id) {
		LOG(stderr, "False Positve in Value Set, Address is 0x%lx\n", address);
		return;
	}

	if (region->base_addr) {
		print_one_region(region);
		LOG(stdout, "Address is 0x%lx\n", address);
		assert(region->base_addr == address);
		return;
	}

	region->base_addr = address;
}

// find region in value set list
re_value_set * find_region_in_valset(re_value_set *valset, re_region_t *reg) {
	re_value_set *result = NULL;
	re_value_set *entry;
	list_for_each_entry(entry, &valset->list, list) {
		if (entry->region == reg) {
			result = entry;
			break;
		}
	}
	return result;
}

re_region_t *select_newest_region(region_type type) {
	// common rule: newest are always in the tail
	re_region_t *region = NULL, *entry;
	list_for_each_entry_reverse(entry, &re_ds.region_head.list, list) {
		if (entry->type == type) {
			region = entry;
			break;
		}
	}
	return region;	
}

void delete_region_list(re_region_t *head) {
	re_region_t *entry, *temp;
	list_for_each_entry_safe(entry, temp, &head->list, list) {
		list_del(&entry->list);
		free(entry);
		entry = NULL;
	}
}

/*
 * API for strided interval
 */

int max_si() {
	// originally the value should be calculated by bit filed
	// in strided_interval
	return LONG_MAX;
}

int min_si() {
	// originally the value should be calculated by bit filed
	// in strided_interval
	return LONG_MIN;
}

bool si_is_one_value(re_si *si) {
	return ((si->stride == 0) && (si->lower_bound == si->upper_bound));
}

bool si_is_full(re_si *si) {
	return ((si->lower_bound == min_si()) && (si->upper_bound == max_si()));
}

void si_set_full(re_si *si) {
	si->lower_bound = min_si();
	si->upper_bound = max_si();
	si->stride = 1;
}

bool si_contain(re_si *si1, re_si *si2) {
	if (si1->stride == 0) {
		if ((si2->stride == 0) &&
		    (si1->lower_bound == si2->lower_bound))
			return true;
		else
			return false;
	}

	if (si2->stride == 0) {
		if IS_IN_SI(si2->lower_bound, si1) 
			return true;
		else
			return false;
	}
	
	if (si_is_full(si1)) {
		return true;
	} else if (si_is_full(si2)) {
		return false;
	}

	// neither si is full stride interval
	int i;
	for (i=si2->lower_bound; i<=si2->upper_bound; i+=si2->stride) {
		if (!IS_IN_SI(i, si1))
			return false;
	}
	return true;
}

bool si_contain_with_len(re_si *si1, int size1, re_si *si2, int size2) {
	if (si_is_full(si1)) return true;

	if (!si_is_full(si1) && si_is_full(si2)) {
		return false;
	}

	if ((si1->stride == 0) && (si2->stride == 0)) {
		if (superset_mem(si1->lower_bound, size1, si2->lower_bound, size2)) {
			return true;
		} else {
			return false;
		}
	}

	int i, j;

	// only stride interval 1 has one value
	if (si1->stride == 0) {
		for (i=si2->lower_bound; i<=si2->upper_bound; i+=si2->stride) {
			if (!superset_mem(si1->lower_bound, size1,
					i, size2)) {
				return false;
			}
		}
		return true;
	}

	// only stride interval 2 has one value
	if (si2->stride == 0) {
		for (i=si1->lower_bound; i<=si1->upper_bound; i+=si1->stride) {
			if (superset_mem(i, size1,
					si2->lower_bound, size2)) {
				return true;
			} 
		}
		return false;
	}

	// FIXME: not full or not one value set
	for (j=si2->lower_bound; j<=si2->upper_bound; j+=si2->stride) {
		for (i=si1->lower_bound; i<=si1->upper_bound; i+=si1->stride) {
			if (!superset_mem(i, size1,
					j, size2)) {
				return false;
			} 
		}
	}
	return true;
}


bool si_nooverlap(re_si *si1, int size1, re_si si2, int size2) {
	assert(0);
}

// verify whether two memory accesses have overlap
bool si_overlap_with_len(re_si *si1, int size1, re_si *si2, int size2) {
	// no overlap if anyone is full si
	if (si_is_full(si1) || si_is_full(si2)) return true;

	// FIXME: subst implementation
	if ((si1->stride == 0) && (si2->stride == 0)) {

		//LOG(stdout, "%lx %d %lx %d\n",
		//	si1->lower_bound, size1, si2->lower_bound, size2);
		if (overlap_mem(si1->lower_bound, size1, si2->lower_bound, size2)) {
			return true;
		} else {
			return false;
		} 
	}

	int i, j;

	// only stride interval 1 has one value
	if (si1->stride == 0) {
		for (i=si2->lower_bound; i<=si2->upper_bound; i+=si2->stride) {
			if (overlap_mem(si1->lower_bound, size1,
					i, size2)) {
				return true;
			}
		}
		return false;
	}

	// only stride interval 2 has one value
	if (si2->stride == 0) {
		for (i=si1->lower_bound; i<=si1->upper_bound; i+=si1->stride) {
			if (overlap_mem(i, (int)size1,
					si2->lower_bound, (int)size2)) {
				return true;
			} 
		}
		return false;
	}
	
	for (i=si1->lower_bound; i<=si1->upper_bound; i+=si1->stride) {
		for (j=si2->lower_bound; j<=si2->upper_bound; j+=si2->stride) {
			if (overlap_mem(i, (int)size1,
					j, (int)size2)) {
				return true;
			} 
		}
	}
	return false;
}

bool si_subset(re_si *si1, re_si *si2) {
	return si_contain(si2, si1);
}

re_si * si_intersection(re_si *si1, re_si *si2) {
	assert(0);
}

re_si * si_union(re_si *si1, re_si *si2) {
	assert(0);
}

// expand one strided interval

/*
 * Arithmetic for strided interval
 */

void si_add_offset(re_si *si, int offset) {
	if (si_is_full(si)) return;
	si->lower_bound += offset;
	si->upper_bound += offset;
}

// take "overflow" into consideration later
// take care that don't use any parameter directly
void si_add(re_si *dest, re_si *src1, re_si *src2) {
	if (si_is_full(src1) || si_is_full(src2)) {
		si_set_full(dest);
		return;
	}

	long offset;

	if (src2->stride == 0) {
		offset = src2->lower_bound;
		memcpy(dest, src1, sizeof(re_si));
		si_add_offset(dest, offset);	
		return;
	}

	if (src1->stride == 0) {
		offset = src1->lower_bound;
		memcpy(dest, src2, sizeof(re_si));
		si_add_offset(dest, offset);	
		return;
	}

	dest->lower_bound = src1->lower_bound + src2->lower_bound;
	dest->upper_bound = src1->upper_bound + src2->upper_bound;

	if (src1->stride != src2->stride) {
		dest->stride = gcd(src1->stride, src2->stride);
	} else {
		dest->stride = src1->stride;
	}
}

void si_sub_offset(re_si *si, int offset) {
	si_add_offset(si, -offset);
}

// sub(src1, src2) = add(src1 + neg(src2))
void si_sub(re_si *dest, re_si *src1, re_si *src2) {
	re_si tmp;
	memcpy(&tmp, src2, sizeof(re_si));
	si_neg(&tmp);

	si_add(dest, src1, &tmp);
}

// take care of overflow
void si_mul_offset(re_si *si, int offset) {
	if (si_is_full(si)) return;
	if (offset > 0) {
		si->lower_bound *= offset;
		si->upper_bound *= offset;
		si->stride *= offset;
	} else {
		int temp = si->lower_bound;
		si->lower_bound = si->upper_bound * offset;
		si->upper_bound = temp * offset;
		si->stride *= offset;
	}
}


void si_shl(re_si *si, re_si *src1, re_si *src2) {
	assert(si_is_one_value(src1) && si_is_one_value(src2));

	if (si_is_full(src1) || si_is_full(src2)) {
		si_set_full(si);
	}

	si->stride = 0;
	si->lower_bound = si->upper_bound = src1->lower_bound << src2->lower_bound;
}

void si_shr(re_si *si, re_si *src1, re_si *src2) {
	assert(si_is_one_value(src1) && si_is_one_value(src2));

	if (si_is_full(src1) || si_is_full(src2)) {
		si_set_full(si);
	}

	si->stride = 0;
	si->lower_bound = si->upper_bound = src1->lower_bound >> src2->lower_bound;
}

static unsigned long minOR(unsigned long lb1, unsigned long ub1, unsigned long lb2, unsigned long ub2) {
	unsigned long m, temp;

	m = 0x80000000;
	while(m != 0) {
		if (~lb1 & lb2 & m) {
			temp = (lb1 | m) & -m;
			if (temp <= ub1) {
				lb1 = temp;
				break;	
			}
		} else if (lb1 & ~lb2 & m){
			temp = (lb2 | m) & -m;
			if (temp <= ub2) {
				lb2 = temp;
				break;	
			}
		}
		m = m >> 1;
	}
	return lb1 | lb2;
}

static unsigned long maxOR(unsigned long lb1, unsigned long ub1, unsigned long lb2, unsigned long ub2) {
	unsigned long m, temp;

	m = 0x80000000;
	while(m != 0) {
		if (ub1 & ub2 & m) {
			temp = (ub1 - m) | (m - 1);
			if (temp >= lb1) {
				ub1 = temp;
				break;	
			}
			temp = (ub2 - m) | (m - 1);
			if (temp >= lb2) {
				ub2 = temp;
				break;	
			}
		}
		m = m >> 1;
	}
	return ub1 | ub2;
}

static int ntz(unsigned long s) {
	int n;
	int y = -s & (s - 1);
	
	n = 0;
	while(y != 0){
		n = n + 1;
		y = y >> 1;
	}
	return n;
}

static bound_info check_bound_status(re_si *src1, re_si *src2) {
	if ((src1->lower_bound < 0) && (src1->upper_bound < 0) &&
	    (src2->lower_bound < 0) && (src2->upper_bound < 0)) {
		return NNNN;
	}
	if ((src1->lower_bound < 0) && (src1->upper_bound < 0) &&
	    (src2->lower_bound < 0) && (src2->upper_bound >= 0)) {
		return NNNP;
	}
	if ((src1->lower_bound < 0) && (src1->upper_bound < 0) &&
	    (src2->lower_bound >= 0) && (src2->upper_bound >= 0)) {
		return NNPP;
	}
	if ((src1->lower_bound < 0) && (src1->upper_bound >= 0) &&
	    (src2->lower_bound < 0) && (src2->upper_bound < 0)) {
		return NPNN;
	}
	if ((src1->lower_bound < 0) && (src1->upper_bound >= 0) &&
	    (src2->lower_bound < 0) && (src2->upper_bound >= 0)) {
		return NPNP;
	}
	if ((src1->lower_bound < 0) && (src1->upper_bound >= 0) &&
	    (src2->lower_bound >= 0) && (src2->upper_bound >= 0)) {
		return NPPP;
	}
	if ((src1->lower_bound >= 0) && (src1->upper_bound >= 0) &&
	    (src2->lower_bound < 0) && (src2->upper_bound < 0)) {
		return PPNN;
	}
	if ((src1->lower_bound >= 0) && (src1->upper_bound >= 0) &&
	    (src2->lower_bound < 0) && (src2->upper_bound >= 0)) {
		return PPNP;
	}
	if ((src1->lower_bound >= 0) && (src1->upper_bound >= 0) &&
	    (src2->lower_bound >= 0) && (src2->upper_bound >= 0)) {
		return PPPP;
	}
}

void si_and_offset(re_si *si, int offset) {
	if (si_is_full(si)) return;

	si->stride = 1;

	if (offset < 0) {
		si->lower_bound = si->lower_bound + offset + 1;	
	} else {
		si->lower_bound = 0;
		si->upper_bound = offset;
	}
}

// si1 & si2 = ~(~si1 | ~si2)
void si_and(re_si *dest, re_si *src1, re_si *src2) {
	
	if ((src1->stride == 0) && (src2->stride == 0)) {
		dest->stride = 0;
		dest->lower_bound = src1->lower_bound & src2->lower_bound;
		dest->upper_bound = dest->lower_bound;
		return;
	}

	re_si tmp_src1, tmp_src2, temp;
	//LOG(stdout, "%lx, %lx\n", src1->lower_bound, src2->lower_bound);

	memcpy(&tmp_src1, src1, sizeof(re_si));
	memcpy(&tmp_src2, src2, sizeof(re_si));

	si_not(&tmp_src1);
	si_not(&tmp_src2);

	si_or(dest, &tmp_src1, &tmp_src2);

	si_not(dest);
}

// si1 ^ si2 = (si1 & ~si2) | (~si1 & si2)
void si_xor(re_si *dest, re_si *src1, re_si *src2) {
	
	if ((src1->stride == 0) && (src2->stride == 0)) {
		dest->stride = 0;
		dest->lower_bound = src1->lower_bound ^ src2->lower_bound;
		dest->upper_bound = dest->lower_bound;
		return;
	}

	re_si tmp_src1, tmp_src2, temp1, temp2;

	memcpy(&tmp_src1, src1, sizeof(re_si));
	memcpy(&tmp_src2, src2, sizeof(re_si));

	si_not(&tmp_src1);
	si_not(&tmp_src2);

	si_and(&temp1, src1, &tmp_src2);
	si_and(&temp2, &tmp_src1, src2);

	si_or(dest, &temp1, &temp2);
}

#define max(a, b) \
	({ typeof (a) _a = (a); \
	   typeof (b) _b = (b); \
	   _a > _b ? _a : _b; })

#define min(a, b) \
	({ typeof (a) _a = (a); \
	   typeof (b) _b = (b); \
	   _a > _b ? _b : _a; })

void si_or(re_si *dest, re_si *src1, re_si *src2) {
	
	if ((src1->stride == 0) && (src2->stride == 0)) {
		//LOG(stdout, "%lx, %lx\n", src1->lower_bound, src2->lower_bound);
		dest->stride = 0;
		dest->lower_bound = src1->lower_bound | src2->lower_bound;
		dest->upper_bound = dest->lower_bound;
		return;
	}
	
	int t = min(ntz(src1->stride), ntz(src2->stride));

	dest->stride = 1 << t;

	int mask, r;
	mask = (1 << t) - 1;	
	r = (src1->lower_bound & mask) | (src1->lower_bound & mask);

	unsigned long tmp_lb, tmp_ub;

	switch (check_bound_status(src1, src2)) {
		case NNNN:
			tmp_lb = minOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			tmp_ub = maxOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			break;
		case NNNP:
			tmp_lb = src1->lower_bound;
			tmp_ub = -1;
			break;
		case NNPP:
			tmp_lb = minOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			tmp_ub = maxOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			break;
		case NPNN:
			tmp_lb = src2->lower_bound;
			tmp_ub = -1;
			break;
		case NPNP:
			tmp_lb = min(src1->lower_bound, src2->lower_bound);
			tmp_ub = maxOR(0, src1->upper_bound,
						  0, src2->upper_bound);
			break;
		case NPPP:
			tmp_lb = minOR(src1->lower_bound, 0xFFFFFFFF,
						  src2->lower_bound, src2->upper_bound);
			tmp_ub = maxOR(0, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			break;
		case PPNN:
			tmp_lb = minOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			tmp_ub = maxOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			break;
		case PPNP:
			tmp_lb = minOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, 0xFFFFFFFF);
			tmp_ub = maxOR(src1->lower_bound, src1->upper_bound,
						  0, src2->upper_bound);
			break;
		case PPPP:
			tmp_lb = minOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			tmp_ub = maxOR(src1->lower_bound, src1->upper_bound,
						  src2->lower_bound, src2->upper_bound);
			break;
	}
	dest->lower_bound = (tmp_lb & (~mask)) | r;
	dest->upper_bound = (tmp_ub & (~mask)) | r;
}

void si_not(re_si *si){
	int tmp = si->upper_bound;
	si->upper_bound = ~(si->lower_bound);
	si->lower_bound = ~(tmp);
}

void si_neg(re_si *si) {
	if (si_is_full(si)) return;
	if ((si->upper_bound == min_si()) &&
	    (si->lower_bound == min_si())) {
		return;
	}
	if (si->lower_bound != min_si()) {
		long tmp = si->upper_bound;
		si->upper_bound = -(si->lower_bound);
		si->lower_bound = -(tmp);
		return;
	}
	si_set_full(si);
}

/*
 * API for value set
 */

// only check absolute equal (reg, stride[low, upper])
bool check_vs_in_list(re_value_set *element, re_value_set *head) {

	if (list_empty(&head->list)) return false;

	re_value_set *entry;
	list_for_each_entry(entry, &head->list, list) {
		if (entry->region != element->region) continue;

		// if the element to be added is subset of any entry, return true
		// we don't need to take care of such entry
		if (si_contain(&element->si, &entry->si)) {
			return true;
		} 
	}
	return false;
}

re_value_set * add_new_valset(re_value_set *head, region_type type, int sub_id, re_si si) {

	re_value_set *new_valset;
	new_valset = (re_value_set *)(malloc(sizeof(re_value_set)));
	if (!new_valset) {
		LOG(stderr, "Value Set malloc error\n");
		return NULL;
	}
	
	memset(new_valset, 0, sizeof(re_value_set));

	print_valset(head);
	LOG(stdout, "region type is %d\n", type);

	new_valset->region = find_region(type, sub_id);
	assert(new_valset->region);
	new_valset->si = si;


	if (!list_empty(&head->list) && (!check_vs_in_list(new_valset, head))) {
		//assert(0 && "Different element");
	}

	// check whether this new element is already contained in the valset
	if (!check_vs_in_list(new_valset, head)) {
		list_add_tail(&new_valset->list, &head->list);
	}

	return new_valset;
}

bool fork_value_set(re_value_set *newhead, re_value_set *oldhead) {

	assert(list_empty(&newhead->list));

	if (list_empty(&oldhead->list))
		return true;

	// traverse all the value set entry and copy them to new node
	re_value_set *entry, *newentry;
	list_for_each_entry_reverse(entry, &oldhead->list, list) {
		newentry = (re_value_set *)(malloc(sizeof(re_value_set)));
		if (!newentry) {
			LOG(stderr, "Malloc Error in fork_value_set\n");
			return false;
		}
		memcpy(newentry, entry, sizeof(re_value_set));
		list_add(&newentry->list, &newhead->list);
	}
	return true;
}

bool delete_value_set(re_value_set *head) {
	if (list_empty(&head->list)) {
		return true;
	}

	re_value_set *entry, *temp;
	list_for_each_entry_safe(entry, temp, &head->list, list) {
		list_del(&entry->list);
		free(entry);
		entry = NULL;
	}

	return true;
}


// this means the linked list is empty
void valset_set_full(re_value_set *valset) {
	re_value_set *ent;
	list_for_each_entry(ent, &valset->list, list) {
		si_set_full(&ent->si);
	}
}

bool valset_is_const(re_value_set *dest) {
	re_value_set *entry;
	entry = list_first_entry(&dest->list, re_value_set, list);

	return ((count_region_num(dest) == 1) &&
		(entry->region->type == CONST_REGION));
}

bool valset_is_empty(re_value_set *dest) {
	return list_empty(&dest->list);
}

// verify whether valset1 contains valset2
// if you want to check whether one valset entry is in another valset,
// just refer to check_vs_in_list
bool valset_contain(re_value_set *valset1, re_value_set * valset2) {

	if (!valset_region_contain(valset1, valset2)) {
		return false;
	}

	re_value_set *entry1, *entry2;
	list_for_each_entry(entry1, &valset1->list, list) {
		entry2 = find_region_in_valset(valset2, entry1->region);
		if (!si_contain(&entry2->si, &entry1->si)) {
			return false;
		}
	}

	return true;
}

re_value_set * valset_intersection(re_value_set *valset1, re_value_set *valset2) {
	assert(0);
	return NULL;
}

re_value_set * valset_union(re_value_set *valset1, re_value_set *valset2) {
	assert(0);
	return NULL;
}

void valset_rm_lowerbound(re_value_set *valset){
	re_value_set *entry;
	list_for_each_entry(entry, &valset->list, list) {
		entry->si.lower_bound = min_si();
	}
}

void valset_rm_upperbound(re_value_set *valset){
	re_value_set *entry;
	list_for_each_entry(entry, &valset->list, list) {
		entry->si.upper_bound = max_si();
	}
}

void valset_modify_region(re_value_set *valset, region_type type, int sub_id) {
	valset->region = find_region(type, sub_id);
	assert(valset->region);
}

/*
 * Arithmetic for Value Set
 */

// add src value set and immediate, and assign the result to dest value set
// if the destination is empty, just fork the src value set to dest value set
void valset_add_offset(re_value_set *dest, re_value_set *src, int offset) {
	if (list_empty(&dest->list)) {
		fork_value_set(dest, src);
	}

	re_value_set *entry;
	list_for_each_entry(entry, &dest->list, list) {
		si_add_offset(&entry->si, offset);
	}
}


void valset_add_si(re_value_set *dest, re_value_set *src, re_si *si) {
	if (list_empty(&dest->list)) {
		fork_value_set(dest, src);
	}

	re_value_set *entry;
	list_for_each_entry(entry, &dest->list, list) {
		// if you modify the second parameter, the first parameter will be modified too
		si_add(&entry->si, &entry->si, si);
	}
}

// please take care of integer overflow here
void valset_add(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	// Region + Constant = Region
	// Constant + Region = Region
	// Constant + Constant = Region
	if (list_empty(&src1->list) || list_empty(&src2->list))
		return;

	re_value_set *ent1;

	if (valset_is_const(src1)) {
		ent1 = list_first_entry(&src1->list, re_value_set, list);
		valset_add_si(dest, src2, &ent1->si);
	} else if (valset_is_const(src2)) {
		ent1 = list_first_entry(&src2->list, re_value_set, list);
		valset_add_si(dest, src1, &ent1->si);
	} else {
		assert(0 && "No implementation here");
	}
}

void valset_sub_offset(re_value_set *dest, re_value_set *src, int offset) {
	valset_add_offset(dest, src, -offset);
}

void valset_sub_si(re_value_set *dest, re_value_set *src1, re_si *si) {
	re_si tmp;
	memcpy(&tmp, si, sizeof(re_si));
	si_neg(&tmp);

	valset_add_si(dest, src1, &tmp);
}

// please take care of integer overflow here
void valset_sub(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	// Region - Region = Constant
	// Region - Constant = Region
	// Constant - Constant = Region
	if (list_empty(&src1->list) || list_empty(&src2->list))
		return;

	re_value_set *ent1, *ent2;
	re_si si;
	int offset;

	if (valset_check_same_region(src1, src2)) {
		list_for_each_entry(ent1, &src1->list, list) {
			ent2 = find_region_in_valset(src2, ent1->region);
			si_sub(&si, &ent1->si, &ent2->si);
			add_new_valset(dest, CONST_REGION, 0, si);	
		}
	//} else if (valset_is_const(src1)) {
	} else if (valset_is_const(src2)) {
		ent1 = list_first_entry(&src2->list, re_value_set, list);
		valset_sub_si(dest, src1, &ent1->si);
	} else {
		assert(0 && "No implementation here");
	}
}

// used in calculating address [ebx + esi * 2 + 0x0C]
void valset_mul_offset(re_value_set *dest, re_value_set *src, int offset) {
	if (list_empty(&dest->list)) {
		fork_value_set(dest, src);
	}

	re_value_set *entry;
	list_for_each_entry(entry, &dest->list, list) {
		// special case : mov edx, [eax+esi*1+0xC]
		// eax : base register, but store index value
		// esi : index register, but store base address
		//assert(entry->region->type == CONST_REGION);
		si_mul_offset(&entry->si, offset);
	}
}

// Product    = Multiplicand * Multiplier
// EDX : EAX  = EAX            EAX
// dst0  dst1 = src0           src1
void valset_mul_one(re_value_set *upper_proc, re_value_set *lower_proc, re_value_set *src1, re_value_set *src2) {
	re_si si;

	if (valset_is_const(src1) && valset_is_const(src2)) {
		si_set_full(&si);
		add_new_valset(upper_proc, CONST_REGION, 0, si);
		add_new_valset(lower_proc, CONST_REGION, 0, si);
	}
/*
	if (valset_is_empty(src1) || valset_is_empty(src2)) {
		si_set_full(&si);
		add_new_valset(upper_proc, CONST_REGION, 0, si);
		add_new_valset(lower_proc, CONST_REGION, 0, si);
	} else if (valset_is_const(src1) && valset_is_const(src2)) {
		// si_mul();
		LOG(stdout, "attention of mul 1\n");
	} else {
		assert(0 && "How to do mul operation for address");
	}
*/
}

void valset_mul_two_or_more(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	re_si si;

	if ((valset_is_const(src1)) && valset_is_const(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	}
/*
	if (valset_is_empty(src1) || valset_is_empty(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	} else if ((valset_is_const(src1)) && valset_is_const(src2)) {
		//si_mul();
		LOG(stdout, "attention of mul 2\n");
	} else {
		assert(0 && "How to do mul operation for address");
	}
*/
}

// Dividend  /  Divisor = Quotient...Remainder
// EDX : EAX    Divisor	  EAX	  ...EDX
//  DX :  AX    Divisor	   AX	  ... DX
//  AH :  AL    Divisor	   AL	  ... AH
void valset_div(re_value_set *quotient, re_value_set *reminder, re_value_set *src1,
		re_value_set *src2, re_value_set *src3) {
	re_si si;

	if (valset_is_const(src1) && valset_is_const(src2) && valset_is_const(src3)) {
		si_set_full(&si);
		add_new_valset(quotient, CONST_REGION, 0, si);
		add_new_valset(reminder, CONST_REGION, 0, si);
	}
/*
	if (valset_is_empty(src1) || valset_is_empty(src2) || valset_is_empty(src3)) {
		si_set_full(&si);
		add_new_valset(quotient, CONST_REGION, 0, si);
		add_new_valset(reminder, CONST_REGION, 0, si);
	} else if (valset_is_const(src1) && valset_is_const(src2) && valset_is_const(src3)) {
		//si_div();
		LOG(stdout, "attention of div\n");
	} else {
		assert(0 && "How to do div operation for address");
	}
*/
}

/*
void valset_shr_si(re_value_set *dest, re_value_set *src1, re_si *si) {
	if (list_empty(&dest->list)) {
		fork_value_set(dest, src);
	}

	re_value_set *entry;
	list_for_each_entry(entry, &dest->list, list) {
		si_shr(&entry->si, &entry->si, si);
	}
}
*/

// only calculate result when src1 is (0[2, 2]), src2 is (0[2, 2])
void valset_shl(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	re_si si;

	if ((valset_is_const(src1)) && valset_is_const(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	}
/*
	re_si *psi1, *psi2;

	if (valset_is_empty(src1) || valset_is_empty(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	} else if ((valset_is_const(src1)) && valset_is_const(src2)) {
		psi1 = &(list_first_entry(&src1->list, re_value_set, list)->si);
		psi2 = &(list_first_entry(&src2->list, re_value_set, list)->si);
		if (si_is_one_value(psi1) && (si_is_one_value(psi2))) {
			LOG(stdout, "attention of shr\n");
			si_shl(&si, psi1, psi2);
			add_new_valset(dest, CONST_REGION, 0, si);
		} else {
			LOG(stdout, "unknown how to handle such shr\n");
		}
	} else {
		assert(0 && "How to do shl operation for address");
	}
*/
}

void valset_shr(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	re_si si;

	if ((valset_is_const(src1)) && valset_is_const(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	}
/*
	if (valset_is_empty(src1) || valset_is_empty(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	} else if ((valset_is_const(src1)) && valset_is_const(src2)) {
		//si_shr();
		LOG(stdout, "attention of shr\n");
	} else {
		assert(0 && "How to do shr operation for address");
	}
*/
}

void valset_ror(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	re_si si;

	if ((valset_is_const(src1)) && valset_is_const(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	}
/*
	if (valset_is_empty(src1) || valset_is_empty(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	} else if ((valset_is_const(src1)) && valset_is_const(src2)) {
		//si_ror();
		LOG(stdout, "attention of ror\n");
	} else {
		assert(0 && "How to do ror operation for address");
	}
*/
}

void valset_rol(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	re_si si;

	if ((valset_is_const(src1)) && valset_is_const(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	}
/*
	if (valset_is_empty(src1) || valset_is_empty(src2)) {
		si_set_full(&si);
		add_new_valset(dest, CONST_REGION, 0, si);
	} else if ((valset_is_const(src1)) && valset_is_const(src2)) {
		//si_rol();
		LOG(stdout, "attention of rol\n");
	} else {
		assert(0 && "How to do rol operation for address");
	}
*/
}

void valset_not(re_value_set *dest, re_value_set *src) {
	if (list_empty(&dest->list)) {
		fork_value_set(dest, src);
	}

	re_value_set *entry;
	list_for_each_entry(entry, &dest->list, list) {
		si_not(&entry->si);
	}
}

// calculate the result only if the two operands are const
void valset_or(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	// Region | Constant = Region => may not exist
	// Constant | Constant = Constant
	if (list_empty(&src1->list) || list_empty(&src2->list))
		return;

	re_si si;
	re_region_t *pregion;
	re_si *psi1, *psi2;

	if ((valset_is_const(src1)) && valset_is_const(src2)) {
		psi1 = &(list_first_entry(&src1->list, re_value_set, list)->si);
		psi2 = &(list_first_entry(&src2->list, re_value_set, list)->si);
		si_or(&si, psi1, psi2);
		LOG(stdout, "result of or here\n");
		add_new_valset(dest, CONST_REGION, 0, si);
		print_valset(dest);
	} else if ((!valset_is_const(src1)) && (valset_is_const(src2))) {
                pregion = list_first_entry(&src1->list, re_value_set, list)->region;
                si_set_full(&si);
                add_new_valset(dest, pregion->type, pregion->sub_id, si);
	} else {
		assert(0 && "No implementation here");
	}
}

// calculate the result only if the two operands are const
void valset_xor(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	// Region ^ Constant = Region => may not exist
	// Constant ^ Constant = Constant
	if (list_empty(&src1->list) || list_empty(&src2->list))
		return;

	re_si si;
	re_si *psi1, *psi2;

	if ((valset_is_const(src1)) && valset_is_const(src2)) {
		psi1 = &(list_first_entry(&src1->list, re_value_set, list)->si);
		psi2 = &(list_first_entry(&src2->list, re_value_set, list)->si);
		si_xor(&si, psi1, psi2);
		LOG(stdout, "result of xor here\n");
		add_new_valset(dest, CONST_REGION, 0, si);
		print_valset(dest);
	} else {
		assert(0 && "No implementation here");
	}
}

// Unary Minus
void valset_neg(re_value_set *valset) {
	re_value_set *entry;
	list_for_each_entry(entry, &valset->list, list) {
		si_neg(&entry->si);
	}
}

void valset_and_offset(re_value_set *dest, re_value_set *src, int offset) {
	if (list_empty(&dest->list)) {
		fork_value_set(dest, src);
	}

	re_value_set *entry;

	if (offset < 0) {
		list_for_each_entry(entry, &dest->list, list) {
			si_and_offset(&entry->si, offset);
		}
	} else {
		list_for_each_entry(entry, &dest->list, list) {
			valset_modify_region(entry, CONST_REGION, 0);
			si_and_offset(&entry->si, offset);
		}
	}
}

void valset_and_si(re_value_set *dest, re_value_set *src, re_si *si) {

	if (list_empty(&dest->list)) {
		fork_value_set(dest, src);
	}

	re_value_set *entry;
	list_for_each_entry(entry, &dest->list, list) {
		si_and(&entry->si, &entry->si, si);
	}
}

void valset_and(re_value_set *dest, re_value_set *src1, re_value_set *src2) {
	// Region & Constant(+) = Constant => may not exist
	// Region & Constant(-) = Region
	// Constant & Constant = Constant
	if (list_empty(&src1->list) || list_empty(&src2->list))
		return;

	re_si *psi1, *psi2;
	int offset;
	re_si si;

	// check constant valset first
	if ((!valset_is_const(src1)) && (valset_is_const(src2))) {
		psi1 = &(list_first_entry(&src1->list, re_value_set, list)->si);
		psi2 = &(list_first_entry(&src2->list, re_value_set, list)->si);
		assert(si_is_one_value(psi2));
		offset = psi2->lower_bound;
		LOG(stdout, "result of address and here\n");
		valset_and_offset(dest, src1, offset);
	} else if ((!valset_is_const(src2)) && (valset_is_const(src1))) {
		psi1 = &(list_first_entry(&src1->list, re_value_set, list)->si);
		psi2 = &(list_first_entry(&src2->list, re_value_set, list)->si);
		assert(si_is_one_value(psi1));
		offset = psi1->lower_bound;
		LOG(stdout, "result of address and here\n");
		valset_and_offset(dest, src2, offset);
	} else if (valset_is_const(src2) && valset_is_const(src1)) {
		psi1 = &(list_first_entry(&src1->list, re_value_set, list)->si);
		psi2 = &(list_first_entry(&src2->list, re_value_set, list)->si);
		LOG(stdout, "result of constant and here\n");
		si_and(&si, psi1, psi2);
		add_new_valset(dest, CONST_REGION, 0, si);
		print_valset(dest);
	} else {
		assert(0 && "No implementation here");
	}
}

// sign extend CONSTANT region value, just for immediate operand
void sign_extend_value_set(re_value_set *head, enum x86_op_datatype datatype) {
	re_value_set *entry;
	char byte;
	short word;

	list_for_each_entry(entry, &head->list, list) {
		assert(entry->region->type == CONST_REGION);
		switch (datatype) {
		case op_byte:
			byte = entry->si.lower_bound & 0xFF;
			if (byte & (1 << (BYTE_SIZE - 1))) {
				entry->si.lower_bound |= 0xFFFFFF00;
				entry->si.upper_bound |= 0xFFFFFF00;
			} else {
				entry->si.lower_bound &= 0xFF;
				entry->si.upper_bound &= 0xFF;
			}
			break;
		case op_word:
			word = entry->si.lower_bound & 0xFFFF;
			if (word & (1 << (WORD_SIZE - 1))) {
				entry->si.lower_bound |= 0xFFFF0000;
				entry->si.upper_bound |= 0xFFFF0000;
			} else {
				entry->si.lower_bound &= 0xFFFF;
				entry->si.upper_bound &= 0xFFFF;
			}
			break;
		case op_dword:
			break;
		default:
			assert(0);
			break;
		}
	}
}

/*
 * compare different memory regions
 */
int count_region_num(re_value_set *head) {
	re_value_set *entry;
	int count = 0;
	list_for_each_entry(entry, &head->list, list)
		count++;
	return count;
}

// get region id from one operand
re_region_t * get_region(re_value_set *head) {
	re_value_set *entry;
	re_region_t *region;
	if (count_region_num(head) > 1) {
		assert(0 && "Point to different regions");
	}
	entry = list_first_entry(&head->list, re_value_set, list);
	return entry->region;
}

// whether valset1 contains valset2
// check if every element in valset2 is in valset1
bool valset_region_contain(re_value_set *valset1, re_value_set *valset2) {
	re_value_set *entry1, *entry2;
	list_for_each_entry(entry2, &valset2->list, list) {
		entry1 = find_region_in_valset(valset1, entry2->region);
		if (!entry1) return false;
	}
	return true;
}

bool valset_region_subset(re_value_set *valset1, re_value_set *valset2) {
	return valset_region_contain(valset2, valset1);
}

bool valset_check_same_region(re_value_set *valset1, re_value_set *valset2) {

	if (count_region_num(valset1) != 
	    count_region_num(valset2)) {
		return false;
	}

	if (valset_region_contain(valset1, valset2) &&
	    valset_region_contain(valset2, valset1)){
		return true;
	} else {
		return false;
	}
}

bool mem_valset_exact(re_value_set *valset1, int size1, re_value_set *valset2, int size2) {

	if (!valset_check_same_region(valset1, valset2)){
		return false;
	}
	
	if(size1 != size2) {
		return false;
	}

	re_value_set *entry1, *entry2;
	list_for_each_entry(entry1, &valset1->list, list) {
		entry2 = find_region_in_valset(valset2, entry1->region);
		if (memcmp(&entry1->si, &entry2->si, sizeof(re_si))) {
			return false;
		}
	}
	return true;
}


bool mem_valset_subst(re_value_set *valset1, int size1, re_value_set *valset2, int size2) {

	if (!valset_region_contain(valset2, valset1)) {
		return false;
	}

	re_value_set *entry1, *entry2;
	list_for_each_entry(entry1, &valset1->list, list) {
		entry2 = find_region_in_valset(valset2, entry1->region);
		if (!si_contain_with_len(&entry2->si, size2, &entry1->si, size1)) {
			return false;
		}
	}
	return true;
}


bool mem_valset_super(re_value_set *valset1, int size1, re_value_set *valset2, int size2) {
	return mem_valset_subst(valset2, size2, valset1, size1);
}


bool mem_valset_overlap(re_value_set *valset1, int size1, re_value_set *valset2, int size2) {
	bool result = false;
	re_value_set *entry1, *entry2;

	if (list_empty(&valset1->list) || list_empty(&valset2->list)) {
		return true;
	}

	list_for_each_entry(entry1, &valset1->list, list) {
		entry2 = find_region_in_valset(valset2, entry1->region);
		// particular case: Global(0) vs Constant
		// they have same base address : 0
		if (!entry2) {
			if (entry1->region->type == GLOBAL_REGION &&
				entry1->region->sub_id == 0) {
				entry2 = find_region_in_valset(valset2, find_region(CONST_REGION, 0));
			}
			if (entry1->region->type == CONST_REGION) {
				entry2 = find_region_in_valset(valset2, find_region(GLOBAL_REGION, 0));
			}
		}
		// if they are all heap regions, no need to check offset, directly set MayAlias
/*
		if (entry2 && (entry1->region->type == HEAP_REGION) && 
                    (entry1->region == entry2->region)) {
			result = true;
			break;
		}
*/
		if (entry2 && si_overlap_with_len(&entry1->si, size1,
						  &entry2->si, size2)) {
			result = true;
			break;
		}
	}
	return result;
}
#endif

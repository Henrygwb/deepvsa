#ifndef __BIN_ALIAS_H__
#define __BIN_ALIAS_H__

void adjust_stack_boundary();
void adjust_func_boundary(re_list_t* inst);
void init_reg_use(re_list_t* usenode, re_list_t* uselist);
bool is_function_start(re_list_t *instnode);

#endif

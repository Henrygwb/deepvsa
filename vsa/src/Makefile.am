bin_PROGRAMS = reverse

# add -DLOG_INSTRUCTIONS to get instruction result

# -Wimplicit-function-declaration : with this warning, you could find implicit function declaration

#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -DDEBUG -I ../include

# 0 means No Alias
AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=0 -DDEBUG -I ../include
# 1 means POMP
#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=1 -DDEBUG -I ../include
# (1 + DL_AST) means POMP
#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=1 -D DL_AST -DDEBUG -I ../include
# 2 means VSA
#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=2 -DDEBUG -I ../include
# (2 + DL_AST) means DL + VSA
#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=2 -D DL_AST -DDEBUG -I ../include
# (2 + HT_AST) means VSA + POMP
#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=2 -D HT_AST -DDEBUG -I ../include
# (2 + DL_AST + HT_AST) means DL + VSA + POMP
#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=2 -D DL_AST -D HT_AST -DDEBUG -I ../include
# 9 means GT
#AM_CFLAGS= -Wimplicit-function-declaration -g -O0 -D ALIAS_MODULE=9 -DDEBUG -I ../include

reverse_CPPFLAGS = -msse4.1

handlers_FILES = handler_arithmetic.c handler_bit_manip.c handler_comparison.c handler_controlflow.c handler_logic.c handler_move.c handler_other.c handler_sse.c handler_stack.c handler_string.c handler_system.c handler_flag_manip.c handler_interrupt.c handler_syscall.c

#handler_interrupt.c

reverse_SOURCES = solver.c access_memory.c alias_manager.c common.c disassemble.c heuristics.c global.c process_binary.c process_core.c process_inst_data.c process_thread.c re_dsmanager.c re_insttable.c re_systable.c re_mem_alias.c re_opdvalue.c reverse_log.c analyze_result.c reverse_execution.c reverse_instructions.c bin_alias.c re_valueset.c re_vsmanager.c re_runtime.c $(handlers_FILES)

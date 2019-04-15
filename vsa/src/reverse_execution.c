#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "reverse_instructions.h"
#include "reverse_log.h"
#include "global.h"
#include "reverse_exe.h"
#include "inst_data.h"
#include "bin_alias.h"
#include "solver.h"
#include "re_runtime.h"

re_t re_ds; 

int main(int argc, char *argv[]){
	
#if !defined (ALIAS_MODULE)
	LOG(stderr, "Please set Alias Module as : \n"
		    "ALIAS_MODULE=0          means No Alias \n"
		    "ALIAS_MODULE=1          means POMP \n"
		    "ALIAS_MODULE=1 + DL_AST means POMP+DL \n"
		    "ALIAS_MODULE=2          means VSA  \n"
		    "ALIAS_MODULE=2 + DL_AST means VSA+DL \n"
		    "ALIAS_MODULE=2 + HT_AST means VSA+POMP \n"
		    "ALIAS_MODULE=2 + DL_AST + HT_AST means VSA+DL+POMP \n"
		    "ALIAS_MODULE=9          means GT+POMP \n");
	return 0;
#endif

	int result;
	size_t instnum, lognum, dlregionnum;
	
	elf_core_info *core_info;
	elf_binary_info *binary_info;
	coredata_t *coredata; 
	x86_insn_t *rawinstlist; 
	operand_val_t *oploglist;
	dlregion_list_t *dlregionlist;

	if (argc != 2) {
		LOG(stderr, "Help: %s testcase_path\n", argv[0]);
		LOG(stderr, "    : Keep all the necessary information \
				in the fold of testcase \n");
		exit(1);
	}

	// initialize the runtime
	init_runtime();

	// pre-processing
	// set path to global path variables
	set_input_data(argv[1]);

	//print_input_data(input_data);

	// parse core dump
	core_info = parse_core(get_core_path()); 
	if (!core_info) {
        	LOG(stderr,"ERROR: The core file is not parsed correctly");
        	exit(1);
	}
	
	// parse binaries
	binary_info = parse_binary(core_info); 
	if (!binary_info) {
		LOG(stderr,"ERROR: The binary file is not parsed correctly");
		exit(1); 
   	} 

	// load data from core dump, including registers and memory
	coredata = load_coredump(core_info, binary_info);
	if (!coredata) {
		LOG(stderr,"ERROR: Cannot load data from core dump");
		exit(1); 
   	}

	print_registers(coredata);

	// load all the instructions in a reversed manner
	instnum = count_linenum(get_inst_path());
	if (instnum < 0) {
		LOG(stderr, "ERROR: read file error when counting linenum\n");
		exit(1);
	}
	
	rawinstlist = (x86_insn_t *)malloc(instnum * sizeof(x86_insn_t));
	if (!rawinstlist) {
		LOG(stderr, "ERROR: malloc error in main\n");
		exit(1);
	}

	memset(rawinstlist, 0, instnum * sizeof(x86_insn_t));
	result = load_trace(core_info, binary_info, get_inst_path(), rawinstlist);

	if (result < 0) {
		LOG(stderr, "ERROR: error in loading all the instructions\n");
		assert(0);
	}

	lognum = count_linenum(get_log_path());
	if (lognum <= 0) {
		LOG(stderr, "Warning: There is no valid log\n");
		lognum = 0;
		oploglist = NULL;
	} else {
		assert("The lognum and instnum must be the same" && instnum == lognum);
		oploglist = (operand_val_t*)malloc(lognum * sizeof(operand_val_t));
		if (!oploglist) {
			LOG(stderr, "ERROR: malloc error\n");
			exit(1);
		}
		
		result = load_log(get_log_path(), oploglist);
		if (result < 0) {
			LOG(stderr, "ERROR: error when loading the data logs\n");
			exit(1);
		}
		assert(instnum == lognum);
	}

        dlregionnum = count_linenum(get_region_path());
        if (dlregionnum <= 0) {
                LOG(stderr, "Warning: There is no valid region from deep learning\n");
                dlregionnum = 0;
                dlregionlist = NULL;
                assert(0);
        } else {
                assert("The lognum and instnum must be the same" && instnum == dlregionnum);
                dlregionlist = (dlregion_list_t *)malloc(instnum * sizeof(dlregion_list_t));

                if (!dlregionlist){
                        LOG(stderr, "ERROR: malloc error in main\n");
                        exit(1);
                }

                memset(dlregionlist, 0, re_ds.instnum * sizeof(dlregion_list_t));
                load_dlregion(get_region_path(), dlregionlist);
                if(result < 0){
                        LOG(stderr, "ERROR: error when loading the memory region from deep learning\n");
                        exit(1);
                }
        }

	// main function of reverse exectuion
	INIT_RE(re_ds, instnum, rawinstlist, coredata, dlregionlist);

	// set up the log of operand values in each instruction
	re_ds.oplog_list.log_num = lognum;
	re_ds.oplog_list.opval_list = oploglist;

	re_ds.dlregionlist = dlregionlist;

	memset(re_ds.flist, 0, MAXFUNC * sizeof(func_info_t));

	reverse_instructions();

	//do some cleanup here
	destroy_instlist(rawinstlist);
	destroy_dlregionlist(dlregionlist);
	destroy_core_info(core_info);
	destroy_bin_info(binary_info);

	clean_input_data(input_data);
	return 0;
}

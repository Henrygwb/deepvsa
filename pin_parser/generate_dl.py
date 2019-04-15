#!/usr/bin/env python

from __future__ import print_function
import sys

# global, stack, heap
maps = []

'''
glob = 0
heap = 1
stack = 2 
others = 3
'''

def which_region(addr):
    region = 3
    for i, lay in enumerate(maps):
        if addr >= lay[0] and addr < lay[1]:
            region = i;
            break;
    return region;

def verify_region(addrlist):
    reg = []
    for addr in addrlist:
        reg.append(which_region(addr))
    return reg;

def parse_file(inst_path, memtrace_path, maps_path, binary_path, region_path, memop_path):

    inst        = []
    memtrace    = []

    binary      = []
    memop_list  = []
    region_list = []

    with open(maps_path, "r") as fr:
        content = fr.readlines()

    assert(len(content) == 3)

    glob_reg  = content[0].strip().split("-")
    heap_reg  = content[1].strip().split("-")
    stack_reg = content[2].strip().split("-")
         
    maps.append([int(glob_reg[0], 16), int(glob_reg[1], 16)])
    maps.append([int(heap_reg[0], 16), int(heap_reg[1], 16)])
    maps.append([int(stack_reg[0], 16), int(stack_reg[1], 16)])

    #for ent in maps :
    #	print(hex(ent[0]), hex(ent[1]))

    with open(inst_path, "r") as fr:
        content = fr.readlines();

    for line in content :
        inst.append(line.strip().split(";"))
          
    with open(memtrace_path, "r") as fr:
        content = fr.readlines()

    for line in content :
        memtrace.append(line.strip().split(":"))

    trace_id = 0

    for ins in inst:
        binary.append(ins[3])
    
        # calculate num to match the length of memtrace
	if int(ins[4]) == 0 :
	    count = 1
        else :
            count = int(ins[4])
        
        memop = []
        for i in range(count) :
            assert(memtrace[trace_id][0] == ins[1])
            memop.append(int(memtrace[trace_id][1], 16))
            trace_id += 1;
        
	if int(ins[4]) == 0 :
	    memop_list.append("")
	else :
	    memop_list.append(":".join(hex(mem) for mem in memop))

        #print("Verfy Region\n")
        reg = verify_region(memop)
        #print("region is ", reg)
        region_list.append(":".join(str(r) for r in reg))
        
    with open(binary_path, "w") as fw:
        fw.write("\n".join([x for x in reversed(binary)]));

    with open(region_path, "w") as fw:
        fw.write("\n".join([str(x) for x in reversed(region_list)]));

    with open(memop_path, "w") as fw:
	fw.write("\n".join([str(x) for x in reversed(memop_list)]))

def main():

    if len(sys.argv) != 7:
        exit(0)
    #          input file 			    | output file
    #          inst.log     memtrace  	 maps         binary       region       memop
    parse_file(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6])

if __name__ == "__main__":
    main()

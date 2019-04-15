#!/usr/bin/env python

import os
import sys

RegIdDict = {"eax" : 1, "ecx": 2, "edx": 3, "ebx": 4, "esp":5, "ebp":6, "esi":7, "edi":8, "ax":9, "cx":10, "dx":11, "bx":12, "sp":13, "bp":14, "si":15, "di":16, "al":17, "cl":18, "dl":19, "bl":20, "ah":21, "ch":22, "dh":23, "bh":24, "mm0":25, "mm1": 26, "mm2": 27, "mm3":28, "mm4":29, "mm5":30, "mm6":31, "mm7":32, "xmm0":33, "xmm1":34, "xmm2":35, "xmm3":36, "xmm4":37, "xmm5":38, "xmm6":39, "xmm7":40, "eflags":81, "eip":85, "es":65, "cs":66, "ss":67, "ds":68, "fs":69, "gs":70, "seg_gs_base":70, "st0":73, "st1":74, "st2":75, "st3":76, "st4":77, "st5":78}

def parse_regs(reginfo):

	regs = {}
	for item in reginfo:
		if item.find("invalid") != -1:
			continue

		regval = item.split(":")
		if RegIdDict[regval[0]] not in regs:
			val = regval[1][2:]
			
			if len(val) % 2:
				val = "0x0" + val
			else:
				val = regval[1]

			regs[RegIdDict[regval[0]]] = val
	return regs

def parse_file(logpath, instpath, regpath):	
	'''
	log format:
	tid;ip;disasm;binary;memop_num;regop_list(name:value)
	'''

	inst = []
	regs = []
	logs = []

	with open(logpath, "r") as fr:
		log = fr.read().split("\n")

	for line in log:
		if not len(line):
			break;

		items = line.split(";")	
		inst.append(items[1])
		
		if len(items) > 5:
			regdict = parse_regs(items[5:])
			if len(regdict) == 0:
				logs.append("noreg")
			else:
				logs.append(";".join([str(reg)+":" + regdict[reg] for reg in regdict]))
		else:
			logs.append("noreg")

	with open(instpath, "w") as fw:
		fw.write("\n".join([x for x in reversed(inst)]))

	with open(regpath, "w") as fw:
		fw.write("\n".join([x for x in reversed(logs)]))

#do the parse
if __name__ == "__main__":
	#          input file | output       file
	# 	   inst.log   | instruction  regstatus
	parse_file(sys.argv[1], sys.argv[2], sys.argv[3])

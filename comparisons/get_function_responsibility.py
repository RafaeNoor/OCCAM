"""
==============================================================
A helper script to print where external function definitions
are available, for library specialization according to specific
main bitcode and a manifest file.

[External function files are generated during the library
specialization passes]

Usage:

python get_function_responsibility.py ext_func_file manifest
==============================================================

"""

import subprocess as sb
import sys
import json

EXT_FILE = sys.argv[1]
MANIFEST = sys.argv[2]
SOURCE_BITCODES = []

with open(MANIFEST,"r") as manFile:
    manifest = json.load(manFile)
    SOURCE_BITCODES = manifest['lib_spec']

functions = {}
with open(EXT_FILE,"r") as readFile:
    functions = json.load(readFile)

functions = functions['functions']

for bc in SOURCE_BITCODES:
    bc_functions = []
    file_name = bc.split(".bc")[0]+"_symbols"
    sb.call("llvm-nm-10 --just-symbol-name --defined-only {0} > {1}".format(bc,file_name), shell=True)

    with open(file_name,"r") as bc_symbols:
        for line in bc_symbols:
            bc_func = line.split("\n")[0]
            if bc_func in functions:
                bc_functions.append(bc_func)
        print("Printing Symbols available in {0}:".format(bc))
        for f in bc_functions:
            print(f)

    print("\n\n")








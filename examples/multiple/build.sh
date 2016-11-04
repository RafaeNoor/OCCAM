#!/usr/bin/env bash

export OCCAM_LOGFILE=${PWD}/previrt/occam.log
export OCCAM_LOGLEVEL=INFO

make clean

mkdir previrt

LIBRARY='library'

unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   LIBRARY='library.so'
elif [[ "$unamestr" == 'Darwin' ]]; then
   LIBRARY='library.dylib'
fi


# Build the manifest file
cat > multiple.manifest <<EOF
{ "modules" : ["main.bc"]
, "binary"  : "main"
, "libs"    : ["${LIBRARY}.bc"]
, "native_libs" : []
, "search"  : []
, "args"    : ["8181"]
, "name"    : "main"
}
EOF

#make the bitcode
CC=wllvm make 
extract-bc main
extract-bc ${LIBRARY}


# Previrtualize
${OCCAM_HOME}/bin/occam previrt --work-dir=previrt multiple.manifest

# Slash
export OCCAM_LOGFILE=${PWD}/slash/occam.log
export OCCAM_LOGLEVEL=INFO
slash --work-dir=slash multiple.manifest

#debugging stuff below:
#for bitcode in previrt/*.bc; do
#    llvm-dis  "$bitcode" &> /dev/null
#done


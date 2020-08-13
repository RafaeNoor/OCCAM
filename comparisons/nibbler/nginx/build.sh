# Download Nginx
echo "Downloading Nginx"
wget http://nginx.org/download/nginx-1.10.3.tar.gz
tar -xf nginx-1.10.3.tar.gz
cd nginx-1.10.3/


# PATCH fix to compile
echo "Applying Patch file to make nginx compile"
cp ../nginx.patch .
patch -p0 < nginx.patch

# Building nginx
CC=gclang ./configure
make -j4
echo "Nginx is built"

# copy bitcode to root
cd objs
get-bc nginx
cp nginx.bc ../../
cd ../../

# building library dependency
# TODO: libpthread and libdl need to be added

python $OCCAM_HOME/comparisons/build_libraries.py libc libcrypto libcrypt libpcre libz


cat > manifest <<EOF
{ "main" : "nginx.bc"
, "binary"  : "nginx_spec"
, "modules"    : []
, "native_libs" : []
, "ldflags" : []
, "name"    : "nginx"
, "args"    : []
, "lib_spec": ["libc.so.bc","libcrypto.so.1.1.bc", "libcrypt.so.1.1.0.bc","libpcre.so.1.2.7.bc", "libz.so.1.2.11.bc"]
, "main_spec": ["nginx.bc"]
}
EOF


export OCCAM_LOGLEVEL=INFO
export OCCAM_LOGFILE=${PWD}/slash/occam.log

slash --no-strip --intra-spec-policy=onlyonce --work-dir=slash  manifest

#debugging stuff below:
for bitcode in slash/*.bc; do
    ${LLVM_HOME}/bin/llvm-dis  "$bitcode" &> /dev/null
done

for bitcode in slash/*-final.bc; do
    ${LLVM_HOME}/bin/llvm-nm  --just-symbol-name "$bitcode" > "$bitcode".nm
done




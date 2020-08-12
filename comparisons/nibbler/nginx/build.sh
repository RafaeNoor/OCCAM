# Download Nginx
wget http://nginx.org/download/nginx-1.10.3.tar.gz
tar -xf nginx-1.10.3.tar.gz
cd nginx-1.10.3/

# PATCH fix to compile
cp ../nginx.patch .
patch -p0 < nginx.patch

# Building nginx
CC=gclang ./configure
make -j4

# copy bitcode to root
cd objs
get-bc nginx
cp nginx.bc ../../




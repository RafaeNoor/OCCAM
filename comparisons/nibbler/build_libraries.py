import subprocess as sb
import sys

LIBRARIES = ["libc","libcrypto","libcrypt","libpcre",
             "libz"]

if len(sys.argv) < 2:
    print("Please specify libraries to build")
    sys.exit(0)

for lib in sys.argv[1:]:
    if lib.lower() not in LIBRARIES:
        print("Library {0} not supported ...".format(lib))
        sys.exit(0)





def build_lib(lib_name, commands):
    with open("build_{}.sh".format(lib_name),"w+") as writeFile:
        for cmd in commands:
            writeFile.write(cmd+"\n")
    sb.call("bash build_{}.sh".format(lib_name),shell=True)
    sb.call("rm build_{}.sh".format(lib_name),shell=True)





def build_libc():
    print("Building libc...")
    commands = [
        "mkdir libc",
        "cd libc",
        "git clone https://github.com/SRI-CSL/musllvm.git",
        "cd musllvm",
        "CC=gclang WLLVM_CONFIGURE_ONLY=1  ./configure --target=LLVM --build=LLVM --prefix=libc_build",
        "make",
        "cd lib",
        "get-bc libc.so"
        "cp libc.so.bc ../../../"
    ]

    build_lib("libc",commands)


def build_libcrypto():
    print("Building libcrypto...")
    commands = [
        "mkdir libcrypto",
        "cd libcrypto",
        "wget https://www.openssl.org/source/old/1.1.1/openssl-1.1.1f.tar.gz",
        "tar -xvzf openssl-1.1.1f.tar.gz",
        "cd openssl-1.1.1f",
        "CC=gclang ./config",
        "make -j2",
        "get-bc libcrypto.so",
        "cp libcrypto.so.1.1.bc ../../"

    ]
    build_lib("libcrypto",commands)

def build_libcrypt():
    print("Building libcrypt...")
    commands = [
        "mkdir libcrypt",
        "cd libcrypt",
        "wget https://mirrors.gandi.net/ubuntu/pool/main/libx/libxcrypt/libxcrypt_4.4.10.orig.tar.xz",
        "tar -xf libxcrypt_4.4.10.orig.tar.xz",
        "d libxcrypt-4.4.10",
        "./bootstrap",
        "CC=gclang ./configure",
        "make -j2",
        "cd ./.libs",
        "get-bc libcrypt.so",
        "cp libcrypt.so.1.1.0.bc ../../../"

    ]
    build_lib("libcrypt",commands)

def build_libpcre():
    print("Building libpcre ...")
    commands = [
        "mkdir libpcre",
        "cd libpcre",
        "wget http://security.ubuntu.com/ubuntu/pool/main/p/pcre3/pcre3_8.39.orig.tar.bz2",
        "tar -xf pcre3_8.39.orig.tar.bz2",
        "cd pcre-8.39",
        "CC=gclang ./configure",
        "make -j2",
        "cd ./.libs",
        "get-bc libpcre.so",
        "cp libpcre.so.1.2.7.bc ../../../"
    ]
    build_lib("libpcre",commands)

def build_libz():
    print("Building libz...")
    commands = [
        "mkdir libz",
        "cd libz",
        "wget https://zlib.net/zlib-1.2.11.tar.gz",
        "tar -xf zlib-1.2.11.tar.gz",
        "cd zlib-1.2.11",
        "CC=gclang ./configure",
        "make",
        "get-bc libz.so",
        "cp libz.so.1.2.11.bc ../../"

    ]
    build_lib("libz",commands)


lib_to_function = {
    "libc": build_libc,
    "libcrypto": build_libcrypto,
    "libcrypt": build_libcrypt,
    "libpcre": build_libpcre,
    "libz": build_libz,
}


for lib in sys.argv[1:]:
    lib_to_function[lib]()


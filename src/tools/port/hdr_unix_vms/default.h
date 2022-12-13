#if ! defined(conf_all_defined)
#define conf_libc6 TRUE
#define conf_DBL TRUE
#define conf_CAS_ENABLED TRUE
#define conf_SVR_AR TRUE
#define conf_WITH_GEO TRUE
#define a64_lnx
#define conf_BUILD_ARCH_64 TRUE
#endif
XTERMLIB = /usr/X11R6/lib64/libXaw.so /usr/X11R6/lib64/libXmu.so /usr/X11R6/lib64/libXt.so /usr/X11R6/lib64/libX11.so /usr/lib64/libtermcap.so
XNETLIB =
NETSCAPE_API_FLAG = -DXP_UNIX
APACHE_API_FLAG = -DLINUX=2
VERS = a64_lnx
VERS32 = int_lnx
VERS64 = a64_lnx
CPP = /lib/cpp
CCMACH32 = -m32 -DBUILD_ARCH32 -fsigned-char -fno-strength-reduce -D_REENTRANT -DLINUX -D_GNU_SOURCE -DXLIB_ILLEGAL_ACCESS  -Wno-write-strings 
CCMACH64 = -m64 -DBUILD_ARCH64 -fno-omit-frame-pointer -fsigned-char -fno-strength-reduce -D_REENTRANT -DLINUX -D_GNU_SOURCE -DXLIB_ILLEGAL_ACCESS  -Wno-write-strings
CCLDMACH32 = -m32 -rdynamic
CCLDMACH64 = -m64 -rdynamic
CCLDSERVER = 
CCPICFLAG = -fPIC
SLSFX = so
MWSLSFX = so
CGISFX = cgi
LDLIBPATH32 = /lib /usr/lib /usr/local/lib
LDLIBPATH64 = /lib64 /usr/lib64 /usr/local/lib64
LIBMACH = 
#define LDLIB -lpthread -lrt -lm -lc -lcrypt -ldl -lgcc_s -lgeos -lgeos_c -lproj
LDLIBMACH32 = -L/usr/local/32bit/lib -L/usr/local/32bit/lib  -lpthread -lrt -lm -lc -lcrypt -ldl -lgcc_s -lgeos -lgeos_c -lproj
LDLIBMACH64 = -L/usr/local/lib -L/usr/local/lib  -lpthread -lrt -lm -lc -lcrypt -ldl -lgcc_s -lgeos -lgeos_c -lproj
LD_ICE_FLAGS = 
CC = cc
OPTIM = -O
LEVEL1_OPTIM = -O1
SHELLHEADER = "#!/bin/sh"
#define HASRANLIB 1
#define XARGS 1
SYS5=1
NO_DIRFUNC=1
INGRES6=1
SYSGRAPHLIB = 
SYMLINK=false
XERCVERS=3.2
#define XERCVERS 3.2
CXX=g++
CXX32_DEFS=-DBUILD_ARCH32
CXX64_DEFS=-DBUILD_ARCH64
XERC_DEFS=-O -DXML_USE_NATIVE_TRANSCODER -DXML_USE_INMEM_MESSAGELOADER -DXML_USE_NETACCESSOR_SOCKET 
XERC_LIBS=
CCMACH="$CCMACH64"
CCLDMACH="$CCLDMACH64"
ASMACH="$ASMACH64"
LDLIBPATH="$LDLIBPATH64"
LDLIBMACH="$LDLIBMACH64"
CXX_DEFS="$CXX64_DEFS"
#define LIB_BLD lib
#define LIB_TGT lib

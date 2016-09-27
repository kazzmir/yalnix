# --------------------------------------------------------------------
# This source distribution is placed in the public domain by its author,
# Jason Papadopoulos. You may use it for any purpose, free of charge,
# without having to notify anyone. I disclaim any responsibility for any
# errors.
# 
# Optionally, please be nice and tell me if you find this source to be
# useful. Again optionally, if you add to the functionality present here
# please consider making those additions public too, so that others may 
# benefit from your work.	
#        				   --jasonp@boo.net 10/3/07  
# --------------------------------------------------------------------

# xlc on AIX; note that apparently a 64-bit binary crashes
# CC = xlc
# OPT_FLAGS = -O2 
# MACHINE_FLAGS = -DRS6K -qmaxmem=8192 -q32

# gcc on Apple G5; for 64-bit mode, add '-m64'
# CC = gcc
# OPT_FLAGS = -O3 -mcpu=970 -mtune=970 -fomit-frame-pointer
# OPT_FLAGS = -O3 -mcpu=7450 -mtune=7450 -fomit-frame-pointer
# WARN_FLAGS = -Wall -W -Wconversion

# gcc with basic optimization (-march flag could
# get overridden by architecture-specific builds)
CC = gcc
WARN_FLAGS = -Wall -W -Wconversion
OPT_FLAGS = -O3 -fomit-frame-pointer -DNDEBUG -march=athlon
# OPT_FLAGS = -O3 -fomit-frame-pointer -DNDEBUG -march=k8

# Note to MinGW users: comment out LIB_THREAD, you don't need it
LIB_THREAD = -lpthread

CFLAGS = $(OPT_FLAGS) $(MACHINE_FLAGS) $(WARN_FLAGS) -Iinclude

#---------------------------------- Generic file lists -------------------

COMMON_HDR = \
	common/lanczos/lanczos.h \
	include/ap.h \
	include/common.h \
	include/dd.h \
	include/fastmult.h \
	include/integrate.h \
	include/msieve.h \
	include/mp.h \
	include/mp_int.h \
	include/util.h

COMMON_SRCS = \
	common/lanczos/lanczos.c \
	common/lanczos/lanczos_matmul.c \
	common/lanczos/lanczos_pre.c \
	common/ap.c \
	common/driver.c \
	common/expr_eval.c \
	common/fastmult.c \
	common/integrate.c \
	common/mp.c \
	common/prime_sieve.c \
	common/rho.c \
	common/squfof.c \
	common/tinyqs.c \
	common/util.c

COMMON_OBJS = $(COMMON_SRCS:.c=.o)

#---------------------------------- QS file lists -------------------------

QS_HDR = mpqs/mpqs.h

QS_SRCS = \
	mpqs/gf2.c \
	mpqs/mpqs.c \
	mpqs/poly.c \
	mpqs/relation.c \
	mpqs/sieve.c \
	mpqs/sieve_core.c \
	mpqs/sqrt.c

QS_OBJS = \
	mpqs/gf2.qo \
	mpqs/mpqs.qo \
	mpqs/poly.qo \
	mpqs/relation.qo \
	mpqs/sieve.qo \
	mpqs/sqrt.qo

QS_CORE_OBJS = \
	mpqs/sieve_core_generic_32k.qo \
	mpqs/sieve_core_generic_64k.qo

QS_CORE_OBJS_X86 = \
	mpqs/sieve_core_p2_64k.qo \
	mpqs/sieve_core_p3_64k.qo \
	mpqs/sieve_core_p4_64k.qo \
	mpqs/sieve_core_pm_32k.qo \
	mpqs/sieve_core_core_32k.qo \
	mpqs/sieve_core_k7_64k.qo \
	mpqs/sieve_core_k7xp_64k.qo \
	mpqs/sieve_core_k8_64k.qo

QS_CORE_OBJS_X86_64 = \
	mpqs/sieve_core_p4_64_64k.qo \
	mpqs/sieve_core_core_64_32k.qo \
	mpqs/sieve_core_k8_64_64k.qo

#---------------------------------- NFS file lists -------------------------

NFS_HDR = \
	gnfs/sqrt/sqrt.h \
	gnfs/poly/poly.h \
	gnfs/poly/skew_sieve.h \
	gnfs/filter/filter.h \
	gnfs/filter/merge_util.h \
	gnfs/gnfs.h

NFS_SRCS = \
	gnfs/poly/poly.c \
	gnfs/poly/polyroot.c \
	gnfs/poly/polyutil.c \
	gnfs/poly/poly_noskew.c \
	gnfs/poly/poly_skew.c \
	gnfs/poly/skew_sieve.c \
	gnfs/filter/clique.c \
	gnfs/filter/duplicate.c \
	gnfs/filter/filter.c \
	gnfs/filter/merge.c \
	gnfs/filter/merge_post.c \
	gnfs/filter/merge_pre.c \
	gnfs/filter/merge_util.c \
	gnfs/filter/singleton.c \
	gnfs/sqrt/sqrt.c \
	gnfs/sqrt/sqrt_a.c \
	gnfs/fb.c \
	gnfs/ffpoly.c \
	gnfs/gf2.c \
	gnfs/gnfs.c \
	gnfs/relation.c \
	gnfs/sieve.c

NFS_OBJS = $(NFS_SRCS:.c=.no)

#---------------------------------- make targets -------------------------

all:
	@echo "pick a target:"
	@echo "x86       32-bit Intel/AMD systems (required if gcc used)"
	@echo "x86_64    64-bit Intel/AMD systems (required if gcc used)"
	@echo "generic   portable code"

x86: $(COMMON_OBJS) $(QS_OBJS) $(QS_CORE_OBJS) \
		$(QS_CORE_OBJS_X86) $(NFS_OBJS)
	rm -f libmsieve.a
	ar r libmsieve.a $(COMMON_OBJS) $(QS_OBJS) \
			$(QS_CORE_OBJS) $(QS_CORE_OBJS_X86) \
			$(NFS_OBJS)
	ranlib libmsieve.a
	$(CC) $(CFLAGS) demo.c -o msieve -lm libmsieve.a $(LIB_THREAD)

x86_64: $(COMMON_OBJS) $(QS_OBJS) $(QS_CORE_OBJS) \
		$(QS_CORE_OBJS_X86_64) $(NFS_OBJS)
	rm -f libmsieve.a
	ar r libmsieve.a $(COMMON_OBJS) $(QS_OBJS) \
			$(QS_CORE_OBJS) $(QS_CORE_OBJS_X86_64) \
			$(NFS_OBJS)
	ranlib libmsieve.a
	$(CC) $(CFLAGS) demo.c -o msieve -lm libmsieve.a $(LIB_THREAD)

generic: $(COMMON_OBJS) $(QS_OBJS) $(QS_CORE_OBJS) $(NFS_OBJS)
	rm -f libmsieve.a
	ar r libmsieve.a $(COMMON_OBJS) $(QS_OBJS) \
			$(QS_CORE_OBJS) $(NFS_OBJS)
	ranlib libmsieve.a
	$(CC) $(CFLAGS) demo.c -o msieve -lm libmsieve.a $(LIB_THREAD)

clean:
	rm -f msieve msieve.exe libmsieve.a $(COMMON_OBJS) 	\
		$(NFS_OBJS) $(QS_OBJS) $(QS_CORE_OBJS) 		\
		$(QS_CORE_OBJS_X86) $(QS_CORE_OBJS_X86_64)

#----------------------------------------- build rules ----------------------

# common file build rules

%.o: %.c $(COMMON_HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# QS build rules

mpqs/sieve_core_generic_32k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -DBLOCK_KB=32 -DCPU_GENERIC \
		-DROUTINE_NAME=qs_core_sieve_generic_32k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_generic_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -DBLOCK_KB=64 -DCPU_GENERIC \
		-DROUTINE_NAME=qs_core_sieve_generic_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_p2_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=pentium2 -DBLOCK_KB=64 -DCPU_PENTIUM2 \
		-DROUTINE_NAME=qs_core_sieve_p2_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_p3_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=pentium3 -DBLOCK_KB=64 -DCPU_PENTIUM3 \
		-DROUTINE_NAME=qs_core_sieve_p3_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_p4_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=pentium4 -DBLOCK_KB=64 -DCPU_PENTIUM4 \
		-DROUTINE_NAME=qs_core_sieve_p4_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_pm_32k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=pentium-m -DBLOCK_KB=32 -DCPU_PENTIUM_M \
		-DROUTINE_NAME=qs_core_sieve_pm_32k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_core_32k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=prescott -DBLOCK_KB=32 -DCPU_CORE \
		-DROUTINE_NAME=qs_core_sieve_core_32k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_k7_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=athlon -DBLOCK_KB=64 -DCPU_ATHLON \
		-DROUTINE_NAME=qs_core_sieve_k7_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_k7xp_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=athlon-xp -DBLOCK_KB=64 -DCPU_ATHLON_XP \
		-DROUTINE_NAME=qs_core_sieve_k7xp_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_k8_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=k8 -DBLOCK_KB=64 -DCPU_OPTERON \
		-DROUTINE_NAME=qs_core_sieve_k8_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_p4_64_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=nocona -DBLOCK_KB=64 -DCPU_PENTIUM4 \
		-DROUTINE_NAME=qs_core_sieve_p4_64k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_core_64_32k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=nocona -DBLOCK_KB=32 -DCPU_CORE \
		-DROUTINE_NAME=qs_core_sieve_core_32k \
		-c -o $@ mpqs/sieve_core.c

mpqs/sieve_core_k8_64_64k.qo: mpqs/sieve_core.c $(QS_HDR)
	$(CC) $(CFLAGS) -march=k8 -DBLOCK_KB=64 -DCPU_OPTERON \
		-DROUTINE_NAME=qs_core_sieve_k8_64k \
		-c -o $@ mpqs/sieve_core.c

%.qo: %.c $(COMMON_HDR) $(QS_HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# NFS build rules

%.no: %.c $(COMMON_HDR) $(NFS_HDR)
	$(CC) $(CFLAGS) -Ignfs -c -o $@ $<

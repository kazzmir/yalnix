#!/bin/sh -x
mkdir -p build/user
mkdir -p build/fs
mkdir -p build/user/file-server

cp src/*.c build
cp user/*.c build/user
cp fs/*.c fs/*.h build/fs
cp user/file-server/*.c build/user/file-server

gcc -o build/user/bash.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/bash.c
gcc -o build/fs/fs.o -c -m32 -Wall -DLINUX -Iinclude build/fs/fs.c
ar rc fs/libfs.a build/fs/fs.o
ranlib fs/libfs.a
gcc -o user/bash -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/bash.o -luser fs/libfs.a
cp "user/bash" "bash"
gcc -o build/debug.o -c -m32 -Wall -DLINUX -Iinclude build/debug.c
gcc -o build/kernel.o -c -m32 -Wall -DLINUX -Iinclude build/kernel.c
gcc -o build/load.o -c -m32 -Wall -DLINUX -Iinclude build/load.c
gcc -o build/memory.o -c -m32 -Wall -DLINUX -Iinclude build/memory.c
gcc -o build/process.o -c -m32 -Wall -DLINUX -Iinclude build/process.c
gcc -o build/schedule.o -c -m32 -Wall -DLINUX -Iinclude build/schedule.c
gcc -o build/user/cat.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/cat.c
gcc -o build/user/checkpoint.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/checkpoint.c
gcc -o build/user/console.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/console.c
gcc -o build/user/copy-from-to.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/copy-from-to.c
gcc -o build/user/cp.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/cp.c
gcc -o build/user/cp1.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/cp1.c
gcc -o build/user/cycle.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/cycle.c
gcc -o build/user/delay.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/delay.c
gcc -o build/user/disk.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/disk.c
gcc -o build/user/dummy.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/dummy.c
gcc -o build/user/echo.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/echo.c
gcc -o build/user/evil.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/evil.c
gcc -o build/user/file-server/file-server.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/file-server/file-server.c
gcc -o build/user/file.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/file.c
gcc -o build/user/fork-bomb.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/fork-bomb.c
gcc -o build/user/fork.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/fork.c
gcc -o build/user/guess.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/guess.c
gcc -o build/user/huge.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/huge.c
gcc -o build/user/init.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/init.c
gcc -o build/user/ipc.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/ipc.c
gcc -o build/user/ln.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/ln.c
gcc -o build/user/ls.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/ls.c
gcc -o build/user/math.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/math.c
gcc -o build/user/memory.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/memory.c
gcc -o build/user/memory_hog.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/memory_hog.c
gcc -o build/user/messaging.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/messaging.c
gcc -o build/user/mkdir.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/mkdir.c
gcc -o build/user/msieve-1.28/common/ap.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/ap.c
gcc -o build/user/msieve-1.28/common/driver.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/driver.c
gcc -o build/user/msieve-1.28/common/expr_eval.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/expr_eval.c
gcc -o build/user/msieve-1.28/common/fastmult.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/fastmult.c
gcc -o build/user/msieve-1.28/common/integrate.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/integrate.c
gcc -o build/user/msieve-1.28/common/lanczos/lanczos.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/lanczos/lanczos.c
gcc -o build/user/msieve-1.28/common/lanczos/lanczos_matmul.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/lanczos/lanczos_matmul.c
gcc -o build/user/msieve-1.28/common/lanczos/lanczos_pre.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/lanczos/lanczos_pre.c
gcc -o build/user/msieve-1.28/common/mp.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/mp.c
gcc -o build/user/msieve-1.28/common/prime_sieve.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/prime_sieve.c
gcc -o build/user/msieve-1.28/common/rho.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/rho.c
gcc -o build/user/msieve-1.28/common/squfof.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/squfof.c
gcc -o build/user/msieve-1.28/common/tinyqs.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/tinyqs.c
gcc -o build/user/msieve-1.28/common/util.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/common/util.c
gcc -o build/user/msieve-1.28/demo.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ibuild/user/msieve-1.28/include build/user/msieve-1.28/demo.c
gcc -o build/user/msieve-1.28/gnfs/fb.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/fb.c
gcc -o build/user/msieve-1.28/gnfs/ffpoly.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/ffpoly.c
gcc -o build/user/msieve-1.28/gnfs/filter/clique.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/clique.c
gcc -o build/user/msieve-1.28/gnfs/filter/duplicate.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/duplicate.c
gcc -o build/user/msieve-1.28/gnfs/filter/filter.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/filter.c
gcc -o build/user/msieve-1.28/gnfs/filter/merge.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/merge.c
gcc -o build/user/msieve-1.28/gnfs/filter/merge_post.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/merge_post.c
gcc -o build/user/msieve-1.28/gnfs/filter/merge_pre.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/merge_pre.c
gcc -o build/user/msieve-1.28/gnfs/filter/merge_util.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/merge_util.c
gcc -o build/user/msieve-1.28/gnfs/filter/singleton.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/filter/singleton.c
gcc -o build/user/msieve-1.28/gnfs/gf2.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/gf2.c
gcc -o build/user/msieve-1.28/gnfs/gnfs.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/gnfs.c
gcc -o build/user/msieve-1.28/gnfs/poly/poly.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/poly/poly.c
gcc -o build/user/msieve-1.28/gnfs/poly/poly_noskew.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/poly/poly_noskew.c
gcc -o build/user/msieve-1.28/gnfs/poly/poly_skew.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/poly/poly_skew.c
gcc -o build/user/msieve-1.28/gnfs/poly/polyroot.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/poly/polyroot.c
gcc -o build/user/msieve-1.28/gnfs/poly/polyutil.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/poly/polyutil.c
gcc -o build/user/msieve-1.28/gnfs/poly/skew_sieve.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/poly/skew_sieve.c
gcc -o build/user/msieve-1.28/gnfs/relation.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/relation.c
gcc -o build/user/msieve-1.28/gnfs/sieve.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/sieve.c
gcc -o build/user/msieve-1.28/gnfs/sqrt/sqrt.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/sqrt/sqrt.c
gcc -o build/user/msieve-1.28/gnfs/sqrt/sqrt_a.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/gnfs/sqrt/sqrt_a.c
gcc -o build/user/msieve-1.28/mpqs/gf2.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/mpqs/gf2.c
gcc -o build/user/msieve-1.28/mpqs/mpqs.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/mpqs/mpqs.c
gcc -o build/user/msieve-1.28/mpqs/poly.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/mpqs/poly.c
gcc -o build/user/msieve-1.28/mpqs/relation.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/mpqs/relation.c
gcc -o build/user/msieve-1.28/mpqs/sieve.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/mpqs/sieve.c
gcc -o build/user/msieve-1.28/mpqs/sqrt.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/mpqs/sqrt.c
gcc -o build/user/msieve-1.28/qs_core_sieve_generic_32k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=32 -DCPU_GENERIC -DROUTINE_NAME=qs_core_sieve_generic_32k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_generic_32k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_generic_64k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=64 -DCPU_GENERIC -DROUTINE_NAME=qs_core_sieve_generic_64k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_generic_64k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_core_32k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=32 -DCPU_GENERIC -DROUTINE_NAME=qs_core_sieve_core_32k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_core_32k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_p4_64k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=64 -DCPU_PENTIUM4 -DROUTINE_NAME=qs_core_sieve_p4_64k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_p4_64k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_p3_64k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=64 -DCPU_PENTIUM3 -DROUTINE_NAME=qs_core_sieve_p3_64k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_p3_64k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_p2_64k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=64 -DCPU_PENTIUM2 -DROUTINE_NAME=qs_core_sieve_p2_64k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_p2_64k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_pm_32k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=32 -DCPU_PENTIUM_M -DROUTINE_NAME=qs_core_sieve_pm_32k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_pm_32k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_k7_64k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=64 -DCPU_ATHLON -DROUTINE_NAME=qs_core_sieve_k7_64k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_k7_64k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_k7xp_64k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=64 -DCPU_ATHLON_XP -DROUTINE_NAME=qs_core_sieve_k7xp_64k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_k7xp_64k/sieve_core.c
gcc -o build/user/msieve-1.28/qs_core_sieve_k8_64k/sieve_core.o -c -m32 -Wall -W -Wconversion -O3 -fomit-frame-pointer -DLINUX -D__ASM__ -DNDEBUG -DBLOCK_KB=64 -DCPU_OPTERON -DROUTINE_NAME=qs_core_sieve_k8_64k -Iinclude -Ibuild/user/msieve-1.28/include -Ibuild/user/msieve-1.28/mpqs -Ibuild/user/msieve-1.28/gnfs -Ibuild/user/msieve-1.28/common build/user/msieve-1.28/qs_core_sieve_k8_64k/sieve_core.c
ar rc build/user/msieve-1.28/libmsieve.a build/user/msieve-1.28/common/lanczos/lanczos.o build/user/msieve-1.28/common/lanczos/lanczos_matmul.o build/user/msieve-1.28/common/lanczos/lanczos_pre.o build/user/msieve-1.28/common/ap.o build/user/msieve-1.28/common/driver.o build/user/msieve-1.28/common/expr_eval.o build/user/msieve-1.28/common/fastmult.o build/user/msieve-1.28/common/integrate.o build/user/msieve-1.28/common/mp.o build/user/msieve-1.28/common/prime_sieve.o build/user/msieve-1.28/common/rho.o build/user/msieve-1.28/common/squfof.o build/user/msieve-1.28/common/tinyqs.o build/user/msieve-1.28/common/util.o build/user/msieve-1.28/mpqs/gf2.o build/user/msieve-1.28/mpqs/mpqs.o build/user/msieve-1.28/mpqs/poly.o build/user/msieve-1.28/mpqs/relation.o build/user/msieve-1.28/mpqs/sieve.o build/user/msieve-1.28/mpqs/sqrt.o build/user/msieve-1.28/gnfs/poly/poly.o build/user/msieve-1.28/gnfs/poly/polyroot.o build/user/msieve-1.28/gnfs/poly/polyutil.o build/user/msieve-1.28/gnfs/poly/poly_noskew.o build/user/msieve-1.28/gnfs/poly/poly_skew.o build/user/msieve-1.28/gnfs/poly/skew_sieve.o build/user/msieve-1.28/gnfs/filter/clique.o build/user/msieve-1.28/gnfs/filter/duplicate.o build/user/msieve-1.28/gnfs/filter/filter.o build/user/msieve-1.28/gnfs/filter/merge.o build/user/msieve-1.28/gnfs/filter/merge_post.o build/user/msieve-1.28/gnfs/filter/merge_pre.o build/user/msieve-1.28/gnfs/filter/merge_util.o build/user/msieve-1.28/gnfs/filter/singleton.o build/user/msieve-1.28/gnfs/sqrt/sqrt.o build/user/msieve-1.28/gnfs/sqrt/sqrt_a.o build/user/msieve-1.28/gnfs/fb.o build/user/msieve-1.28/gnfs/ffpoly.o build/user/msieve-1.28/gnfs/gf2.o build/user/msieve-1.28/gnfs/gnfs.o build/user/msieve-1.28/gnfs/relation.o build/user/msieve-1.28/gnfs/sieve.o build/user/msieve-1.28/qs_core_sieve_generic_32k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_generic_64k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_core_32k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_p4_64k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_p3_64k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_p2_64k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_pm_32k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_k7_64k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_k7xp_64k/sieve_core.o build/user/msieve-1.28/qs_core_sieve_k8_64k/sieve_core.o
ranlib build/user/msieve-1.28/libmsieve.a
gcc -o build/user/msieve-1.28/msieve -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/msieve-1.28/demo.o -luser build/user/msieve-1.28/libmsieve.a -lm
gcc -o build/user/pause.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/pause.c
gcc -o build/user/pid.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/pid.c
gcc -o build/user/ping-server.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/ping-server.c
gcc -o build/user/ping.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/ping.c
gcc -o build/user/portal.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/portal.c
gcc -o build/user/recursion.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/recursion.c
gcc -o build/user/rm.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/rm.c
gcc -o build/user/shell.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/shell.c
gcc -o build/user/simple-io.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/simple-io.c
gcc -o build/user/stack.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/stack.c
gcc -o build/user/stat.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/stat.c
gcc -o build/user/sync.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/sync.c
gcc -o build/user/sysv.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/sysv.c
gcc -o build/user/time.o -c -m32 -DLINUX -D__ASM__ -Iinclude build/user/time.c
gcc -o build/user/touch.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/touch.c
gcc -o build/user/walk.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/walk.c
gcc -o build/user/zero.o -c -m32 -DLINUX -D__ASM__ -Iinclude -Ifs build/user/zero.c
gcc -o user/cat -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/cat.o -luser fs/libfs.a
cp "user/cat" "cat"
gcc -o user/console -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/console.o -luser
cp "user/console" "console"
gcc -o user/cp -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/cp.o -luser fs/libfs.a
cp "user/cp" "cp"
gcc -o user/cp1 -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/cp1.o -luser fs/libfs.a
cp "user/cp1" "cp1"
gcc -o user/echo -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/echo.o -luser fs/libfs.a
cp "user/echo" "echo"
gcc -o user/fileserver -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/file-server/file-server.o -luser fs/libfs.a
cp "user/fileserver" "fileserver"
gcc -o user/init -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/init.o -luser fs/libfs.a
cp "user/init" "init"
gcc -o user/ln -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/ln.o -luser fs/libfs.a
cp "user/ln" "ln"
gcc -o user/ls -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/ls.o -luser fs/libfs.a
cp "user/ls" "ls"
gcc -o user/mkdir -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/mkdir.o -luser fs/libfs.a
cp "user/mkdir" "mkdir"
gcc -o user/portal -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/portal.o -luser fs/libfs.a
cp "user/portal" "portal"
gcc -o user/rm -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/rm.o -luser fs/libfs.a
cp "user/rm" "rm"
gcc -o user/shell -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/shell.o -luser
cp "user/shell" "shell"
gcc -o user/stat -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/stat.o -luser fs/libfs.a
cp "user/stat" "stat"
gcc -o user/sync -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/sync.o -luser fs/libfs.a
cp "user/sync" "sync"
gcc -o user/touch -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/touch.o -luser fs/libfs.a
cp "user/touch" "touch"
gcc -o user/checkpoint -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/checkpoint.o -luser
gcc -o user/copy-from-to -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/copy-from-to.o -luser
gcc -o user/cycle -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/cycle.o -luser
gcc -o user/delay -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/delay.o -luser
gcc -o user/disk -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/disk.o -luser
gcc -o user/dummy -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/dummy.o -luser
gcc -o user/evil -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/evil.o -luser
gcc -o user/file -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/file.o -luser fs/libfs.a
gcc -o user/fork -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/fork.o -luser
gcc -o user/fork-bomb -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/fork-bomb.o -luser
gcc -o user/guess -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/guess.o -luser
gcc -o user/huge -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/huge.o -luser
gcc -o user/ipc -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/ipc.o -luser
gcc -o user/math -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/math.o -luser
gcc -o user/memory -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/memory.o -luser
gcc -o user/memory_hog -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/memory_hog.o -luser
gcc -o user/messaging -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/messaging.o -luser
cp "build/user/msieve-1.28/msieve" "user/msieve"
gcc -o user/pause -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/pause.o -luser
gcc -o user/pid -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/pid.o -luser
gcc -o user/ping -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/ping.o -luser
gcc -o user/ping-server -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/ping-server.o -luser
gcc -o user/recursion -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/recursion.o -luser
gcc -o user/simple-io -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/simple-io.o -luser
gcc -o user/stack -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/stack.o -luser
gcc -o user/sysv -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/sysv.o -luser fs/libfs.a
gcc -o user/time -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/time.o -luser
gcc -o user/walk -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/walk.o -luser fs/libfs.a
gcc -o user/zero -m32 -static -Wl,-T,/home/cs5460/projects/yalnix/public/etc/user.x -u exit -u __brk -u __sbrk -u __mmap -u __default_morecore -L /home/cs5460/projects/yalnix/public/lib build/user/zero.o -luser fs/libfs.a
gcc -o yalnix -Wl,-T,/home/cs5460/projects/yalnix/public/etc/kernel.x -Wl,-R/home/cs5460/projects/yalnix/public/lib -m32 build/kernel.o build/memory.o build/debug.o build/load.o build/process.o build/schedule.o -L/home/cs5460/projects/yalnix/public/lib -lkernel -lhardware -lelf
cp "user/zero" "zero"

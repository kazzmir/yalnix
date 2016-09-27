## Scons build file for yalnix
## www.scons.org
## Jon Rafkind

import os

env = Environment( ENV = os.environ )

yalnix_cs5460_path = '/home/cs5460/projects/yalnix/public'

env.BuildDir( 'build', 'src' )

sources = Split("""
build/kernel.c
build/memory.c
build/debug.c
build/load.c
build/process.c
build/schedule.c
""");

env.Append( CPPPATH = 'include' )
# env.Append( CFLAGS = ['-g3','-ggdb', '-m32','-Wall','-Werror'] )
# env.Append( CFLAGS = ['-g3','-ggdb', '-m32','-Wall'] )
env.Append( CFLAGS = ['-m32','-Wall'] )
env.Append( CPPDEFINES = 'LINUX' )

env.Append( LIBPATH = yalnix_cs5460_path + '/lib' )
env.Append( LIBS = [ 'kernel', 'hardware', 'elf' ] )
env.Append( LINKFLAGS = '-Wl,-T,%s/kernel.x' % (yalnix_cs5460_path + '/etc') )
env.Append( LINKFLAGS = '-Wl,-R%s' % (yalnix_cs5460_path + '/lib') )
env.Append( LINKFLAGS = '-m32' )

env.Program( 'yalnix', sources )

env.Clean( 'yalnix', ['TRACE', 'TTYLOG', 'TTYLOG.0', 'TTYLOG.1', 'TTYLOG.2', 'TTYLOG.3','build'] )

fsEnv = Environment( ENV = os.environ )
fsSource = Split("""
build/fs/fs.c
""");
fsEnv.BuildDir( 'build/fs', 'fs' )
fsEnv.Append( CPPPATH = 'include' )
# fsEnv.Append( CFLAGS = ['-g3','-m32','-Wall'] )
fsEnv.Append( CFLAGS = ['-m32','-Wall'] )
fsEnv.Append( CPPDEFINES = 'LINUX' )
fslib = fsEnv.StaticLibrary( 'fs/fs', fsSource )

userEnv = Environment( ENV = os.environ )
userEnv.Append( CPPDEFINES = ['LINUX', '__ASM__'] )
# userEnv.Append( CFLAGS = ['-g', '-m32'] )
userEnv.Append( CFLAGS = ['-m32'] )
userEnv.Append( CPPPATH = ['#include'] )
userEnv.Append( LINKFLAGS = '-m32' )
userEnv.Append( LINKFLAGS = '-static' )
userEnv.Append( LINKFLAGS = '-Wl,-T,%s/user.x' % (yalnix_cs5460_path + '/etc') )
userEnv.Append( LINKFLAGS = '-u exit' )
userEnv.Append( LINKFLAGS = '-u __brk' )
userEnv.Append( LINKFLAGS = '-u __sbrk' )
userEnv.Append( LINKFLAGS = '-u __mmap' )
userEnv.Append( LINKFLAGS = '-u __default_morecore' )
userEnv.Append( LINKFLAGS = '-L %s' % (yalnix_cs5460_path + '/lib') )
userEnv.Append( LIBS = 'user' )

# LDFLAGS = -static -Wl,-T,$(ETCDIR)/user.x -u exit -u __brk -u __sbrk -u __mmap -L $(LIBDIR) -luser -D__ASM__ -u __default_morecore
userEnv.BuildDir( 'build/user', 'user' )
userEnv.Program( 'user/checkpoint', 'build/user/checkpoint.c' )
userEnv.Program( 'user/dummy', 'build/user/dummy.c' )
userEnv.Program( 'user/memory_hog', 'build/user/memory_hog.c' )
userEnv.Program( 'user/huge', 'build/user/huge.c' )
userEnv.Install( '.', userEnv.Program( 'user/console', 'build/user/console.c' ))
userEnv.Install( '.', userEnv.Program( 'user/shell', 'build/user/shell.c' ))
userEnv.Program( 'user/recursion', 'build/user/recursion.c' )
userEnv.Program( 'user/pause', 'build/user/pause.c' )
userEnv.Program( 'user/math', 'build/user/math.c' )
userEnv.Program( 'user/fork', 'build/user/fork.c' )
userEnv.Program( 'user/memory', 'build/user/memory.c' )
userEnv.Program( 'user/delay', 'build/user/delay.c' )
userEnv.Program( 'user/fork-bomb', 'build/user/fork-bomb.c' )
userEnv.Program( 'user/simple-io', 'build/user/simple-io.c' )
userEnv.Program( 'user/messaging', 'build/user/messaging.c' )
userEnv.Program( 'user/pid', 'build/user/pid.c' )
userEnv.Program( 'user/guess', 'build/user/guess.c' )
userEnv.Program( 'user/stack', 'build/user/stack.c' )
userEnv.Program( 'user/time', 'build/user/time.c' )
userEnv.Program( 'user/ping-server', 'build/user/ping-server.c' )
userEnv.Program( 'user/ping', 'build/user/ping.c' )
userEnv.Program( 'user/evil', 'build/user/evil.c' )
userEnv.Program( 'user/ipc', 'build/user/ipc.c' )
userEnv.Program( 'user/cycle', 'build/user/cycle.c' )
userEnv.Program( 'user/copy-from-to', 'build/user/copy-from-to.c' )
userEnv.Program( 'user/disk', 'build/user/disk.c' )
userEnv.InstallAs( 'user/msieve', SConscript( 'user/msieve-1.28/SConstruct', build_dir = 'build/user/msieve-1.28', exports = 'userEnv' ) )

fsUserEnv = userEnv.Copy()
fsUserEnv.Append( LIBS = [fslib] )
fsUserEnv.Append( CPPPATH = '#./fs' )
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/fileserver', 'build/user/file-server/file-server.c' ) )

fsUserEnv.Program( 'user/file', 'build/user/file.c' )
fsUserEnv.Program( 'user/walk', 'build/user/walk.c' )
fsUserEnv.Program( 'user/sysv', 'build/user/sysv.c' )
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/bash', 'build/user/bash.c' ) )
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/ls', 'build/user/ls.c' ) )
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/mkdir', 'build/user/mkdir.c' ) )
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/init', 'build/user/init.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/touch', 'build/user/touch.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/portal', 'build/user/portal.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/zero', 'build/user/zero.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/rm', 'build/user/rm.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/stat', 'build/user/stat.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/echo', 'build/user/echo.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/cat', 'build/user/cat.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/ln', 'build/user/ln.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/cp', 'build/user/cp.c' ))
fsUserEnv.Install( '.', fsUserEnv.Program( 'user/sync', 'build/user/sync.c' ))

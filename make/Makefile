# Makefile Template

# PUBDIR for .eng.utah.edu machines
PUBDIR = /home/cs5460/projects/yalnix/public

# PUBDIR for .cs.utah.edu machines
#PUBDIR = /uusoc/sys/classes/cs5460/projects/yalnix/public

ALL = yalnix

# Many of the src, object and header files below are just suggestions,
# as they hint at how one could break their project into multiple files.
# Remove any you don't want or need.
KERNEL_SRCS = src/kernel.c
KERNEL_OBJS = src/kernel.o
KERNEL_INCS = 

YALNIX_OUTPUT = yalnix

CC = gcc

LIBDIR = $(PUBDIR)/lib
INCDIR = $(PUBDIR)/include
ETCDIR = $(PUBDIR)/etc

# any extra loading flags...
LD_EXTRA = 

KERNEL_CFLAGS = -g -m32 -DLINUX 
KERNEL_LIBS = $(LIBDIR)/libkernel.a $(LIBDIR)/libhardware.so 
CPPFLAGS=-I. -I$(INCDIR)

# the "kernel.x" argument tells the loader to use the memory layout
# in the kernel.x file..
KERNEL_LDFLAGS = $(LD_EXTRA) -L$(LIBDIR) -lkernel -lelf -Wl,-T,$(ETCDIR)/kernel.x -Wl,-R$(LIBDIR) -lhardware
LINK_KERNEL = $(LINK.c) $(KERNEL_CFLAGS) 

all: $(ALL)

clean:
	rm -f *.o *~ TTYLOG* TRACE $(YALNIX_OUTPUT) $(USER_APPS)

count:
	wc $(KERNEL_SRCS)

list:
	ls -l *.c *.h

kill:
	killall yalnixtty

yalnix: $(KERNEL_OBJS) $(KERNEL_LIBS) $(KERNEL_INCS)
	$(LINK_KERNEL) -o yalnix $(KERNEL_OBJS) $(KERNEL_LDFLAGS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(KERNEL_CFLAGS) -c $(@:%.o=%.c)

# For user programs, make a userprog sub-directory, and put all 
# user programs in there. You also need to create a makefile in
# that directory using Makefile.User template

makeuserprog:
	cd userprog ; make


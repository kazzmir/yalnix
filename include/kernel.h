#ifndef _yalnix_kernel_h
#define _yalnix_kernel_h

#include "debug.h"
#include "process.h"

/* for a sender
 *  to: pid of process to send to
 *  from:  pid of process sending
 *  receive: -1
 * for a receiver
 *  to: -1
 *  from: pid of process that is sending or -1
 *  receive: pid of process receiving
 */
struct ipc{
	int to;
	int from;
	int receive;
};

int loadProgram( struct process * process, char *name, char *args[] );

#endif

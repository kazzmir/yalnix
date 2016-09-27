#ifndef _yalnix_debug_h
#define _yalnix_debug_h

#include "hardware.h"

/* yalnix errors */
typedef enum {
	YALNIX_NO_ERROR = 0,
	YALNIX_TRAP_SETUP_ERROR = 1,
	YALNIX_NO_FREE_VIRTUAL_PAGES = 2,
	YALNIX_CANNOT_LOAD_PROGRAM = 3,
	YALNIX_INVALID_PROGRAM = 4,
	YALNIX_OUT_OF_MEMORY = 5,
	YALNIX_INVALID_CHILD = 6,
} YalnixError;

/* dump a kernel context to a file named 'name' */
void dumpKernelContext( const char * name, KernelContext * context );

/* print an error in human readable form */
void printError( YalnixError error );

#endif

#include "hardware.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

/* print all the values in a UserContext structure
 */
 /*
static void printContext( UserContext * context ){
	int reg;
	printf( "context[ %p ]\n", context );
	printf( "  vector[ %d ]\n", context->vector );
	printf( "  code[ %d ]\n", context->code );
	printf( "  addr[ %p ]\n", context->addr );
	printf( "  pc[ %p ]\n", context->pc );
	printf( "  sp[ %p ]\n", context->sp );
#ifdef LINUX
	printf( "  ebp[ %p ]\n", context->ebp );
#endif
	for ( reg = 0; reg < GREGS; reg++ ){
		printf( "  reg%d[ %lu ]\n", reg, context->regs[ reg ] );
	}
}
*/

/* Print a register in human readable form.
 * It doesn't seem like we really need this, but maybe
 * it will be useful later.
 *
static const char * registerToName( int reg ){
	switch( reg ){
		case REG_EAX : return "eax";
		case REG_EBX : return "ebx";
		case REG_ECX : return "ecx";
		case REG_EDX : return "edx";
		case REG_ESI : return "esi";
		case REG_EDI : return "edi";
		case REG_EIP : return "eip";
		case REG_ESP : return "esp";
		case REG_ERR : return "err";
		case REG_TRAPNO : return "trapno";
		case REG_EBP : return "ebp";
		default : return "unknown";
	}
}
*/

void dumpKernelContext( const char * name, KernelContext * context ){
	FILE * f = fopen( name, "w" );
	if ( f ){
		char buf[ 1024 ];
		memcpy( buf, context, sizeof(*context) );
		fwrite( buf, 1, sizeof(*context), f );
		fclose( f );
	}
}

/*
static void printKernelContext( KernelContext * context ){
	printf( "Context %p\n", context );
	printf( "uc_flags %lu\n", context->uc_flags );
	printf( "uc_link %p\n", context->uc_link );
	printf( "uc_stack %d\n", context->uc_stack );
	printf( "uc_mcontext %d\n", context->uc_mcontext );
	printf( "uc_sigmask %d\n", context->uc_sigmask );
	printf( "fpregs %d\n", context->__fpregs_mem );
}
*/

/* print yalnix error( an enum ) in human readable form */
void printError( YalnixError error ){
	switch ( error ){
		case YALNIX_TRAP_SETUP_ERROR : {
			printf( "Could not set up trap handlers\n" );
			break;
		}
		case YALNIX_INVALID_CHILD : {
			printf( "Parent notified of death from unrelated child\n" );
			break;
		}
		case YALNIX_NO_FREE_VIRTUAL_PAGES : {
			printf( "No more free virtual pages\n" );
			break;
		}
		default : {
			printf( "Unknown error code %d\n", error );
		}
	}
}


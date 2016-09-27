/* try to do some evil things like get the os to touch memory we don't own
 * If any of the Send() calls hang, meaning the process is waiting for a
 * reply, then the OS did not check the memory address properly.
 */

#include "yalnix.h"
#include "hardware.h"

int main(){
	char * buffer = (char *) VMEM_1_LIMIT;	
	TtyPrintf( 0, "Trying to send data from VMEM_1_LIMIT %p\n", buffer );
	if ( Send( buffer, GetPid() + 1 ) == ERROR ){
		TtyPrintf( 0, "OS correctly returned error\n" );	
	}
	buffer = VMEM_1_BASE;
	TtyPrintf( 0, "Trying to send data from VMEM_1_BASE %p\n", buffer );
	if ( Send( buffer, GetPid() + 1 ) == ERROR ){
		TtyPrintf( 0, "OS correctly returned error\n" );	
	}
	buffer = VMEM_0_BASE;
	TtyPrintf( 0, "Trying to send data from VMEM_0_BASE %p\n", buffer );
	if ( Send( buffer, GetPid() + 1 ) == ERROR ){
		TtyPrintf( 0, "OS correctly returned error\n" );
	}
	buffer = VMEM_1_LIMIT - 16;
	TtyPrintf( 0, "Trying to send data from VMEM_1_BASE - 16 %p\n", buffer );
	if ( Send( buffer, GetPid() + 1 ) == ERROR ){
		TtyPrintf( 0, "OS correctly returned error\n" );
	}
	buffer = &buffer - PAGESIZE * 2;
	TtyPrintf( 0, "Trying to send data from somewhere in the heap %p\n", buffer );
	if ( Send( buffer, GetPid() + 1 ) == ERROR ){
		TtyPrintf( 0, "OS correctly returned error\n" );
	}

	TtyPrintf( 0, "Trying to ttywrite into bogus memory\n" );
	if ( TtyWrite( 0, 0xae, 4 ) == ERROR ){
		TtyPrintf( 0, "OS correctly returned error\n" );
	}

	char buf[ 32 ];

	int pid = Fork();
	switch ( pid ){
		case 0 : {
			Delay( 5 );
			break;
		}
		case ERROR : {
			TtyPrintf( 0, "evil: Could not fork child\n" );
			break;
		}
		default : {
			char b2[ 32 ];
			TtyPrintf( 0, "Copy from: %d\n", CopyFrom( pid, b2, buf, 32 ) ); 
			TtyPrintf( 0, "Copy to: %d\n", CopyTo( pid, buf, b2, 32 ) );
			break;
		}
	}

	return 0;
}

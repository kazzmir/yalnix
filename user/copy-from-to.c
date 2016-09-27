/* test the CopyFrom and CopyTo sys calls */

#include "yalnix.h"
#include "hardware.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
	int parent = GetPid();
	int child = Fork();
	if ( child == ERROR ){
		TtyPrintf( 0, "Could not fork\n" );
		return 0;
	}
	if ( child != 0 ){
		unsigned char * buffer = malloc( PAGESIZE * 2 );	
		bzero( buffer, PAGESIZE * 2 );
		char message[ 32 ];
		buffer[ 0 ] = 12;
		buffer[ 100 ] = 32;
		buffer[ 1000 ] = 96;
		buffer[ PAGESIZE + 23 ] = 101;
		buffer[ PAGESIZE + 850 ] = 19;
		int id = ReceiveSpecific( message, child );
		char * addr = *(char **) message;
		TtyPrintf( 0, "Received something from %d\n", id );
		if ( CopyTo( child, addr, buffer, PAGESIZE * 2 ) == ERROR ){
			TtyPrintf( 0, "Error with copy to\n" );
			return -1;
		}
		char * addr2 = *((char **)message + 1);
		if ( CopyFrom( child, buffer, addr2, PAGESIZE * 2 ) == ERROR ){
			TtyPrintf( 0, "Error with copy from\n" );
			return -1;
		}
		TtyPrintf( 0, "%d [0] should be 4. got %d\n", parent, buffer[ 0 ] );
		TtyPrintf( 0, "%d [6000] should be 98. got %d\n", parent, buffer[ 6000 ] );
		TtyPrintf( 0, "%d [PAGESIZE + 100] should be 203. got %d\n", parent, buffer[ PAGESIZE + 100 ] );
		Reply( message, id );
	} else {
		int id = GetPid();
		/* force an offset in buffer so it doesn't match up with the parent buffer */
		malloc( 64 );
		char * buffer = malloc( PAGESIZE * 2 );
		unsigned char * buffer2 = malloc( PAGESIZE * 2 );
		bzero( buffer, PAGESIZE * 2 );
		bzero( buffer2, PAGESIZE * 2 );
		char message[ 32 ];
		*(char **)message = buffer;
		*((unsigned char **)message+1) = buffer2;

		buffer2[ 0 ] = 4;
		buffer2[ 6000 ] = 98;
		buffer2[ PAGESIZE + 100 ] = 203;

		Send( message, parent );
		TtyPrintf( 0, "%d [0] should be 12. got %d\n", id, buffer[ 0 ] );
		TtyPrintf( 0, "%d [100] should be 32. got %d\n", id, buffer[ 100 ] );
		TtyPrintf( 0, "%d [1000] should be 96. got %d\n", id, buffer[ 1000 ] );
		TtyPrintf( 0, "%d [PAGESIZE + 23] should be 101. got %d\n", id, buffer[ PAGESIZE + 23 ] );
		TtyPrintf( 0, "%d [PAGESIZE + 850] should be 19. got %d\n", id, buffer[ PAGESIZE + 850 ] );
	}

	return 0;
}

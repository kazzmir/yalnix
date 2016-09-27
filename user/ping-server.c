#include "yalnix.h"
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char ** argv ){
	
	int port = 100;
	if ( argc > 1 ){
		port = atoi( argv[ 1 ] );
	}

	TtyPrintf( 0, "Starting a ping server on port %d\n", port );
	if ( Register( port ) == ERROR ){
		TtyPrintf( 0, "Port %d already in use\n", port );
		return -1;
	}

	int myid = GetPid();
	int quit = 0;
	while ( ! quit ){
		char buffer[ 32 ];
		int pid = Receive( buffer );
		if ( pid != ERROR ){
			buffer[ 31 ] = 0;
			TtyPrintf( 0, "Server received '%s' from %d\n", buffer, pid );
			quit = strcasecmp( buffer, "quit" ) == 0;
			snprintf( buffer, 32, "ping from server %d", myid );
			if ( Reply( buffer, pid ) == ERROR ){
				TtyPrintf( 0, "Server couldn't send reply to %d\n", pid );
			}
		} else {
			TtyPrintf( 0, "Server couldn't receive message\n" );
		}
	}

	TtyPrintf( 0, "Server releasing port %d\n", port );

	return 0;
}

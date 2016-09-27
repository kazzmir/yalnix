#include "yalnix.h"
#include <string.h>

int main( int argc, char ** argv ){
	if ( argc < 3 ){
		return -1;
	}

	int port = atoi( argv[ 1 ] );
	char message[ 32 ];
	strncpy( message, argv[ 2 ], 30 );
	message[ 31 ] = 0;
	if ( Send( message, -port ) == ERROR ){
		TtyPrintf( 0, "%s could not send message to port %d\n", argv[ 0 ], port );
	} else {
		TtyPrintf( 0, "%s received '%s' from server\n", argv[ 0 ], message );
	}

	return 0;
}

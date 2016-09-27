#include "fs.h"
#include "yalnix.h"
#include <stdio.h>

int main( int argc, char ** argv ){
	if ( argc < 4 ){
		return 0;
	}

	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );
	char * name = argv[ 3 ];
	Sync( Lookup( name, dir ) );

	return 0;
}

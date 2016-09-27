#include "fs.h"
#include "yalnix.h"
#include <stdio.h>

int main( int argc, char ** argv ){
	if ( argc < 5 ){
		return 0;
	}

	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );
	char * src = argv[ 3 ];
	char * dest = argv[ 4 ];

	CreateLink( dest, Lookup( src, dir ), dir );

	return 0;
}

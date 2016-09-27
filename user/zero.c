#include "fs.h"
#include "filesystem.h"
#include "yalnix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main( int argc, char ** argv ){
	if ( argc < 4 ){
		return 0;
	}

	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );
	char * name = argv[ 3 ];
	int bytes = atoi( argv[ 4 ] );
	CreateFile( name, dir );

	int max = SECTORSIZE * 10;
	int offset = 0;

	while ( bytes > 0 ){
		int size = bytes > max ? max : bytes;
		char * buffer = (char *) malloc( size );
		bzero( buffer, size );
		Write( Lookup( name, dir ), size, offset, buffer );
		free( buffer );
		offset += size;
		bytes -= size;
	}

	return 0;
}

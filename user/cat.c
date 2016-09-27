#include "fs.h"
#include "yalnix.h"
#include <stdio.h>

void cat( int terminal, char * file, FH_t dir ){
	char data[ 1024 ];
	int bytes = 0;
	int offset = 0;
	FH_t id = Lookup( file, dir );
	if ( id != -1 ){
		do{
			bytes = Read( id, 1022, offset, data );
			if ( bytes > 0 ){
				data[ bytes + 1 ] = 0;
				TtyPrintf( terminal, "%s", data );
				offset += bytes;
			}
		} while ( bytes > 0 );
	}
	TtyPrintf( terminal, "\n" );
}

int main( int argc, char ** argv ){
	if ( argc < 4 ){
		return 0;
	}

	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );
	int i;
	for ( i = 3; i < argc; i++ ){
		cat( terminal, argv[ i ], dir );
	}

	return 0;
}

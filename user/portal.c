#include "fs.h"
#include "yalnix.h"
#include <stdio.h>
#include <stdlib.h>

void copy( int terminal, char * file, FH_t id ){
	char data[ 4096 ];
	FILE * f = fopen( file, "rb" );
	if ( ! f ){
		return;
	}

	int offset = 0;
	int bytes = 0;
	while ( ! feof( f ) ){
		bytes = fread( data, 1, 4096, f );
		if ( bytes > 0 ){
			Write( id, bytes, offset, data );
			offset += bytes;
		}
	}

	fclose( f );
}

int main( int argc, char ** argv ){
	if ( argc < 5 ){
		return 0;
	}

	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );
	char * real_world = argv[ 3 ];
	char * yalnix_file = argv[ 4 ];
	CreateFile( yalnix_file, dir );
	FH_t made = Lookup( yalnix_file, dir );
	if ( made != ERROR ){
		copy( terminal, real_world, made );
	}

	return 0;
}

#include "fs.h"
#include "yalnix.h"
#include <stdio.h>
#include <string.h>

int main( int argc, char ** argv ){
	if ( argc < 5 ){
		return 0;
	}

	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );
	char * src = argv[ 3 ];
	char * dest = argv[ 4 ];

	FH_t src_id = Lookup( src, dir );
	if ( src_id == ERROR ){
		return -1;
	}

	TtyPrintf( terminal, "Copy %s to %s\n", src, dest );
	if ( IsDirectory( Lookup( dest, dir ) ) ){
		dir = Lookup( dest, dir );
		dest = strrchr( src, '/' );
		if ( dest == 0 ){
			dest = src;
		} else {
			dest += 1;
		}
	}

	Delete( dest, dir );
	CreateFile( dest, dir );
	FH_t dest_id = Lookup( dest, dir );
	if ( dest_id == ERROR ){
		return -1;
	}

	if ( src_id == dest_id ){
		return -1;
	}

	char data[ 4096 ];
	int offset = 0;
	int bytes = 0;
	do{
		bytes = Read( src_id, 4096, offset, data );
		if ( bytes > 0 ){
			TtyPrintf( terminal, "Copy %d bytes from %d to %d\n", bytes, src_id, dest_id );
			Write( dest_id, bytes, offset, data );
			offset += bytes;
		}
	} while ( bytes > 0 );

	return 0;
}

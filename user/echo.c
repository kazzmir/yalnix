#include "fs.h"
#include "yalnix.h"
#include <stdio.h>
#include <string.h>

void echo_write( char * string, char * file, FH_t dir ){
	Delete( file, dir );
	CreateFile( file, dir );
	Write( Lookup( file, dir ), strlen( string ), 0, string );
}

void echo_append( char * string, char * file, FH_t dir ){
	FH_t id = Lookup( file, dir );
	Write( id, strlen( string ), Stat( id ), string );
}

int main( int argc, char ** argv ){
	if ( argc < 3 ){
		return 0;
	}
	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );

	char buf[ 4096 ];
	buf[ 0 ] = 0;
	int i;
	for ( i = 3; i < argc; i++ ){
		if ( strcmp( argv[ i ], ">" ) == 0 ){
			i += 1;
			if ( i < argc ){
				echo_write( buf, argv[ i ], dir );
			}
			return 0;
		} else if ( strcmp( argv[ i ], ">>" ) == 0 ){
			i += 1;
			if ( i < argc ){
				echo_append( "\n", argv[ i ], dir );
				echo_append( buf, argv[ i ], dir );
			}
			return 0;
		} else {
			strcat( buf, argv[ i ] );
			strcat( buf, " " );
		}
	}
	TtyPrintf( terminal, "%s\n", buf );

	return 0;
}

#include "fs.h"
#include "yalnix.h"
#include <stdio.h>

void walk( FH_t dir, char * base ){
	// TtyPrintf( 0, "[file] Walk %d\n", dir );
	int offset = 0;
	char * name = readdir( dir, &offset );
	char all[ 128 ];
	while ( name != 0 ){
		TtyPrintf( 0, "[file] Found '%s/%s'\n", base, name );
		sprintf( all, "%s/%s", base, name );
		if ( !(strcmp( "..", name ) == 0 || strcmp( ".", name ) == 0 ) ){
			walk( Lookup( name, dir ), all );
		}
		name = readdir( dir, &offset );
	}
}

int main(){
	walk( Lookup( "/", 0 ), "" );
	return 0;
}

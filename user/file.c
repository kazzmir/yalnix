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
		walk( Lookup( name, dir ), all );
		name = readdir( dir, &offset );
	}
}

int main(){
	Delay( 1 );
	TtyPrintf( 0, "[file] create file = %d\n", CreateFile( "/hello", 0 ) );
	TtyPrintf( 0, "[file] create file = %d\n", CreateFile( "/foo-bar", 0 ) );
	TtyPrintf( 0, "[file] create file = %d\n", CreateFile( "/bar/foo-bar", 0 ) );

	TtyPrintf( 0, "[file] Handle for /hello = %d\n", Lookup( "/hello", 0 ) );

	FH_t root = Lookup( "/", 0 );

	TtyPrintf( 0, "[file] Found root at %d\n", root );

	CreateDir( "bob", root );
	TtyPrintf( 0, "[file] Found bob at %d\n", Lookup( "/bob", 0 ) );

	CreateFile( "foo", Lookup( "/bob", 0 ) );

	walk( Lookup( "/", 0 ), "" );

	Delete( "/bob/foo", 0 );
	Delete( "hello", Lookup( "/", 0 ) );
	TtyPrintf( 0, "[file] Create /test1 is %d\n", CreateDir( "/test1", 0 ) );
	TtyPrintf( 0, "[file] Create /test1/test2 is %d\n", CreateDir( "/test1/test2", 0 ) );
	TtyPrintf( 0, "[file] /test1 is %d\n", Lookup( "/test1", 0 ) );
	TtyPrintf( 0, "[file] /test1/test2 is %d\n", Lookup( "/test1/test2", 0 ) );
	CreateFile( "x", Lookup( "/test1/test2", 0 ) );
	Delete( "test2", Lookup( "/test1", 0 ) );
	Delete( "x", Lookup( "/test1/test2", 0 ) );
	TtyPrintf( 0, "[file] Delete /test1/test2 is %d\n", Delete( "test2", Lookup( "/test1", 0 ) ) );
	TtyPrintf( 0, "[file] /test1/test2 is %d\n", Lookup( "/test1/test2", 0 ) );
	TtyPrintf( 0, "[file] /bogus is %d\n", Lookup( "/bogus", 0 ) );
	CreateFile( "/test1/foo", 0 );
	TtyPrintf( 0, "[file] /test1/foo at %d\n", Lookup( "/test1/foo", 0 ) );
	TtyPrintf( 0, "[file] /test1/foo at %d\n", Lookup( "//../../../test1/.././test1/foo", 0 ) );
	TtyPrintf( 0, "[file] /test1/foo at %d\n", Lookup( "/./test1/../test1/foo", 0 ) );

	Shutdown();

	return 0;
}

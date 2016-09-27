#include "yalnix.h"

int main( int argc, char ** argv ){
	int num = 1;
	int i;
	char * prog = "user/dummy";

	if ( argc > 1 ){
		num = atoi( argv[ 1 ] );
	}
	if ( argc > 2 ){
		prog = argv[ 2 ];
	}

	while( num ){
		num -= 1;
		if ( Fork() == 0 ){
			char * buf[ 2 ];
			buf[ 0 ] = prog;
			buf[ 1 ] = 0;
			TtyPrintf( 0, "fork-bomb: Created %d\n", GetPid() );
			Exec( prog, buf );
		}
	}

	return 0;
}

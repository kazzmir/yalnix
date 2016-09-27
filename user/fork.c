#include <stdio.h>
#include "yalnix.h"

void make( const char * program ){
	int pid = 0;
	printf( "Fork %s\n", program );
	switch ( pid = Fork() ){
		case ERROR : {
			printf( "Fork broked\n" );
			break;
		}
		case 0 : {
			char * buf[ 5 ];
			buf[ 0 ] = "f1";
			buf[ 1 ] = "f2";
			buf[ 2 ] = "f3";
			buf[ 3 ] = "popsicles";
			buf[ 4 ] = 0;
			printf( "I am the child %s pid %d\n", program, pid );
			Exec( program, buf );
			printf( "%s failed to exec\n", program );
			break;
		}
		default : {
			printf( "I am the parent. child pid for %s is %d\n", program, pid );
			break;
		}
	}
}

int main(){
	int pid = 0;
	int status = 0;
	int i;

	make( "user/checkpoint" );
	make( "user/delay" );
	make( "user/math" );

	for ( i = 0; i < 3; i++ ){
		pid = Wait( &status );
		if ( pid != ERROR ){
			printf( "%d'th dead child %d status %d\n", i + 1, pid, status );
		} else {
			printf( "No more dead children, wait %d failed\n", i );
		}
	}
	return 0;
}

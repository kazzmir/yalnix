/* $ time some-process
 * Shows how long some-process took to execute including time for fork and exec.
 * Useful for benchmarking how much load the os is under or just timing how long
 * a process took.
 */

#include <stdlib.h>
#include <sys/time.h>
#include "yalnix.h"

/* return time in microseconds */
unsigned long long now(){
	struct timeval t;
	gettimeofday( &t, NULL );
	return t.tv_sec * 1000 * 1000 + t.tv_usec;
}

static void showTime( unsigned long long show ){
	TtyPrintf( 0, "%f seconds / %f milliseconds\n", (double) show / ( 1000.0 * 1000.0 ), (double) show / 1000.0 );
}

int main( int argc, char ** argv ){
	if ( argc > 1 ){
		unsigned long long start;
		start = now();
		switch ( Fork() ){
			/* child */
			case 0 : {
				char ** args = malloc( sizeof(char*) * argc - 1 + 1 );
				int i;
				if ( ! args ){
					TtyPrintf( 0, "Could not malloc buffer for args\n" );
					return -1;
				}
				for ( i = 1; i < argc; i++ ){
					args[ i - 1 ] = argv[ i ];
				}
				args[ i - 1 ] = 0;
				if ( Exec( args[ 0 ], args ) == ERROR ){
					TtyPrintf( 0, "Could not exec %s\n", argv[ 1 ] );
				}
				break;
			}
			case ERROR : {
				TtyPrintf( 0, "%s could not fork\n", argv[ 0 ] );
				break;
			}
			/* parent */
			default : {
				int status;

				start = now();
				if ( Wait( &status ) != ERROR ){
					showTime( now() - start );
					TtyPrintf( 0, "%s exited with status %d\n", argv[ 1 ], status );
				} else {
					TtyPrintf( 0, "%s died\n", argv[ 1 ] );
				}
				break;
			}
		}
	}
	return 0;
}

/* make sbrk() go up and down
 */

#include <stdlib.h>

int main(){
	char * n1 = malloc( 0x20000 );
	char * n2 = malloc( 0x20000 );
	char * n3 = malloc( 0x20000 );
	char * n4 = malloc( 0x20000 );
	char * n5 = malloc( 0x20000 );
	printf( "Free n5\n" );
	free( n5 );
	printf( "Free n4\n" );
	free( n4 );
	printf( "Free n3\n" );
	free( n3 );
	printf( "Free n2\n" );
	free( n2 );
	printf( "Free n1\n" );
	free( n1 );
}

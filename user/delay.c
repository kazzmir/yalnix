/* call the Delay syscall a few times
 */

#include "yalnix.h"

int main(){
	int n;
	for ( n = 0; n < 3; n++ ){
		TtyPrintf( 0, "Hello from delay\n" );
		Delay( 4 );
	}
	return 0;
}

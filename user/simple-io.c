/* test io
 */
#include "yalnix.h"

#define MAX_LINE 1024

int main(){
	char buf[ MAX_LINE ];
	TtyPrintf( 1, "Testing\n");
	int n = TtyRead( 1, buf, MAX_LINE );
	TtyPrintf( 0, "simple-io: Read %d bytes\n", n );
	buf[ n ] = 0;
	TtyPrintf( 0, "simple-io: %s\n", buf );
}

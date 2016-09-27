/* test the GetPid() syscall
 */

#include "yalnix.h"

int main( int argc, char ** argv ){
	TtyPrintf( 0, "Pid is %d\n", GetPid() );	
}

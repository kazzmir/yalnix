/* set up sysv file system */

#include "fs.h"

int main(){
	TtyPrintf( 0, "Create system V hierarchy\n" );
	CreateDir( "/bin", 0 );
	CreateDir( "/tmp", 0 );
	CreateDir( "/etc", 0 );
	CreateDir( "/home", 0 );
	CreateDir( "/var", 0 );
	Shutdown();
}

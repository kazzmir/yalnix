#include "yalnix.h"
#include "hardware.h"
#include <string.h>

int main(){

	char buffer[ SECTORSIZE ];
	bzero( buffer, SECTORSIZE );

	buffer[ 10 ] = 54;

	TtyPrintf( 0, "[10] = %d\n", buffer[ 10 ] );
	TtyPrintf( 0, "Write: %d\n", WriteSector( 1, buffer ) );
	// Delay( 2 );
	bzero( buffer, SECTORSIZE );
	TtyPrintf( 0, "[10] = %d\n", buffer[ 10 ] );
	TtyPrintf( 0, "Read: %d\n", ReadSector( 1, buffer ) );
	// Delay( 2 );
	TtyPrintf( 0, "[10] = %d\n", buffer[ 10 ] );
}

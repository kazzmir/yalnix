/* Example usage of ReceiveSpecific. output should be exactly in this order
 * child1
 * child1
 * child2
 * child1
 *
 * Anything else is an error.
 */

#include "yalnix.h"
#include <stdio.h>
#include <stdlib.h>

void try( int x ){
	if ( x == ERROR ){
		TtyPrintf( 0, "Failed\n" );
		exit( -1 );
	}
}

void do1( int parent ){
	char buffer[ 32 ];
	int id = GetPid();
	sprintf( buffer, "hello 1 from %d", id );
	try( Send( buffer, parent ) );
	sprintf( buffer, "hello 2 from %d", id );
	try( Send( buffer, parent ) );
	sprintf( buffer, "hello 3 from %d", id );
	try( Send( buffer, parent ) );
}

void do2( int parent ){
	char buffer[ 32 ];
	int id = GetPid();
	sprintf( buffer, "hello 1 from %d", id );
	try( Send( buffer, parent ) );
}

int main(){
	int child1;
	int child2;
	int myid = GetPid();

	child1 = Fork();
	if ( child1 == 0 ){
		do1( myid );
		return 0;
	} else {
		child2 = Fork();
		if ( child2 == 0 ){
			do2( myid );
			return 0;
		}
	}

	char message[ 32 ];
	Delay( 2 );
	TtyPrintf( 0, "Receiving from %d\n", child1 );
	try( ReceiveSpecific( message, child1 ) );
	TtyPrintf( 0, "Received '%s'\n", message );
	sprintf( message, "Ok" );
	try( Reply( message, child1 ) );
	try( Delay( 2 ) );
	try( ReceiveSpecific( message, child1 ) );
	try( Reply( message, child1 ) );
	TtyPrintf( 0, "Received '%s'\n", message );
	try( Delay( 2 ) );
	try( ReceiveSpecific( message, child2 ) );
	try( Reply( message, child2 ) );
	TtyPrintf( 0, "Received '%s'\n", message );
	try( Delay( 2 ) );
	try( ReceiveSpecific( message, child1 ) );
	try( Reply( message, child1 ) );
	TtyPrintf( 0, "Received '%s'\n", message );
}

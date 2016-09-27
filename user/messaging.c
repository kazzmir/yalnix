#include <stdio.h>
#include "yalnix.h"

const int max_messages = 5;

void receive( char * name ){
	int num = 1;
	int myid = GetPid();
	while ( num < max_messages ){
		char message[ 32 ];
		TtyPrintf( 0, "%s:%d receiving\n", name, myid );
		int pid = Receive( message );
		if ( pid != ERROR ){
			TtyPrintf( 0, "%s:%d received '%s' from %d\n", name, myid, message, pid );
		} else {
			TtyPrintf( 0, "%s:%d could not receive\n" );
		}
		sprintf( message, "thanks %d\n", num );
		if ( Reply( message, pid ) != ERROR ){
			TtyPrintf( 0, "%s:%d sent a reply to %d\n", name, myid, pid );
		} else {
			TtyPrintf( 0, "%s:%d could not send reply to %d\n", name, myid, pid );
		}
		num += 1;
	}
}

void send( char * program, int pid ){
	char message[ 32 ];
	int num = 1;
	int myid = GetPid();
	while ( num < max_messages ){
		sprintf( message, "hello world %d", num );
		TtyPrintf( 0, "%s:%d sending '%s'\n", program, myid, message );
		if ( Send( message, pid ) != ERROR ){
			TtyPrintf( 0, "%s:%d received %s\n", program, myid, message );
		} else {
			TtyPrintf( 0, "%s:%d could not send\n", program, myid );
		}
		num += 1;
	}
}

int main( int argc, char ** argv ){
	int pid = Fork();
	switch( pid ){
		case 0 : {
			receive( "receiver" );
			break;
		}
		case ERROR : {
			TtyPrintf( 0, "%s could not fork\n", argv[ 0 ] );
			break;
		}
		default : {
			send( "sender", pid );
			break;
		}
	}
	return 0;
}

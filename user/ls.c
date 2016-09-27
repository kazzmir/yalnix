#include "fs.h"
#include "yalnix.h"
#include <stdio.h>
#include <time.h>

char * showType( int type ){
	switch( type ){
		case 0 : return "file";
		case 1 : return "dir";
		default : return "";
	}
}

char * showTime( time_t time ){
	static char buf[ 128 ];
	strftime( buf, 128, "%F %r", localtime( &time ) );
	return buf;
}

void showFile( FH_t id, int terminal, int listing ){
	char path[ 1024 ];
	GetPath( path, 1024, id );
	if ( listing ){
		int size = Stat( id );
		int regular = 0;
		unsigned int time = Stamp( id );
		TtyPrintf( terminal, "%s\t\t%s\t%d bytes\tLast Modified %s", path, showType( regular ), size, showTime( time ) );
	}
	TtyPrintf( terminal, "\n" );
}

void walk( FH_t dir, int terminal, int recurse, int listing ){
	int offset = 0;
	char * name = readdir( dir, &offset );
	char path[ 1024 ];
	GetPath( path, 1024, dir );
	while ( name != 0 ){
		TtyPrintf( terminal, "%s%s", path, name );
		if ( listing ){
			FH_t id = Lookup( name, dir );
			int size = Stat( id );
			int regular = IsDirectory( id );
			unsigned int time = Stamp( id );
			TtyPrintf( terminal, "\t\t%s\t%d bytes\tLast Modified %s", showType( regular ), size, showTime( time ) );
		}
		TtyPrintf( terminal, "\n" );
		if ( recurse && !(strcmp(name,"..") == 0 || strcmp(name,".") == 0) ){
			walk( Lookup( name, dir ), terminal, recurse, listing );
		}
		name = readdir( dir, &offset );
	}
}

void usage( int terminal ){
	TtyPrintf( terminal, "-r : recurse down directories\n" );
	TtyPrintf( terminal, "-l : show file size, date, and possibly other attributes\n" );
	TtyPrintf( terminal, "-h : this help\n" );
}

int main( int argc, char ** argv ){
	if ( argc < 3 ){
		return 0;
	}

	int terminal = atoi( argv[ 1 ] );
	FH_t dir = atoi( argv[ 2 ] );
	int recurse = 0;
	int listing = 0;

	int i;
	FH_t newdir = dir;

	for ( i = 3; i < argc; i++ ){
		if ( strcasecmp( argv[ i ], "-r" ) == 0 ){
			recurse = 1;
		} else if ( strcasecmp( argv[ i ], "-l" ) == 0 ){
			listing = 1;
		} else if ( strcmp( argv[ i ], "-h" ) == 0 ){
			usage( terminal );
			return 0;
		} else {
			newdir = Lookup( argv[ i ], dir );
		}
	}

	dir = newdir;

	if ( IsDirectory( dir ) ){
		walk( dir, terminal, recurse, listing );
	} else {
		showFile( dir, terminal, listing );
	}

	return 0;
}

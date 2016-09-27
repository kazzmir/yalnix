#include "fs.h"
#include "yalnix.h"
#include "filesystem.h"

#include <string.h>

static int send( void * data ){
	if ( Send( data, -FILE_SERVER ) == ERROR ){
		return ERROR;
	}
	return 0;
}

int CreateFile( char * filename, FH_t dir ){
	struct fs_message_create_file message;
	message.type = CREATE_FILE;
	message.filename = filename;
	message.filename_length = strlen( filename );
	message.dir = dir;
	message.code = ERROR;

	TtyPrintf( 0, "Create file from %p\n", filename );

	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int CreateLink( char * link, FH_t file, FH_t dir ){
	struct fs_message_create_link message;
	message.type = CREATE_LINK;
	message.link = link;
	message.length = strlen( link );
	message.dir = dir;
	message.file = file;
	
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int CreateDir( char * dirname, FH_t dir ){
	struct fs_message_create_dir message;

	message.type = CREATE_DIR;
	message.name = dirname;
	message.length = strlen( dirname );
	message.dir = dir;

	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int Delete( char * objname, FH_t dir ){
	struct fs_message_delete message;

	message.obj = objname;
	message.length = strlen( objname );
	message.dir = dir;
	message.type = DELETE;
	
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int Read( FH_t file, int count, int offset, char * buffer ){
	struct fs_message_read message;

	message.type = READ;
	message.file = file;
	message.count = count;
	message.offset = offset;
	message.buffer = buffer;

	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int Write( FH_t file, int count, int offset, char * buffer ){
	struct fs_message_write message;

	message.type = WRITE;
	message.file = file;
	message.count = count;
	message.offset = offset;
	message.buffer = buffer;
	
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int Stat( FH_t file ){
	struct fs_message_stat message;

	message.type = STAT;
	message.file = file;
	
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int Stamp( FH_t file ){
	struct fs_message_stamp message;

	message.type = STAMP;
	message.file = file;
	
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int Sync( FH_t file ){
	struct fs_message_sync message;

	message.type = SYNC;
	message.file = file;
	
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

void SyncDisk(void){
	struct fs_message_sync_disk message;
	message.type = SYNC_DISK;
	
	send( &message );
}

int Shutdown(void){
	struct fs_message_shutdown message;
	message.code = ERROR;
	message.type = SHUTDOWN;
	
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

char * readdir( FH_t dir, int * offset ){
	struct fs_message_readdir message;
	static char name[ DIRNAMELEN ];

	message.type = READDIR;
	message.dir = dir;
	message.offset = *offset;
	message.buffer = name;

	if ( send( &message ) != 0 ){
		return 0;
	}

	*offset = message.offset;
	if ( message.code == 1 ){
		return name;
	} else {
		return 0;
	}
}

FH_t Lookup( char * path, FH_t dir ){
	struct fs_message_lookup message;
	message.type = LOOKUP;
	message.name = path;
	message.length = strlen( path );
	message.dir = dir;
	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.result;
}

int GetPath( char * path, int max_length, FH_t dir ){
	struct fs_message_get_path message;
	message.type = GET_PATH;

	message.path = path;
	message.length = max_length;
	message.dir = dir;

	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

int IsDirectory( FH_t id ){
	struct fs_message_is_directory message;
	message.type = IS_DIRECTORY;
	message.id = id;

	if ( send( &message ) != 0 ){
		return ERROR;
	}

	return message.code;
}

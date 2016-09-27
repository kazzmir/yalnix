#ifndef _fs_eb9083de27794aafbd36e65f4c86a506_h
#define _fs_eb9083de27794aafbd36e65f4c86a506_h

#include <stdint.h>

typedef uint32_t FH_t;

enum fs_operation{
	NONE,
	CREATE_FILE,
	CREATE_LINK,
	CREATE_DIR,
	DELETE,
	READ,
	WRITE,
	STAT,
	STAMP,
	SYNC,
	SYNC_DISK,
	SHUTDOWN,
	READDIR,
	LOOKUP,
	GET_PATH,
	IS_DIRECTORY,
};

/*
struct fs_message{
	enum fs_operation operation;
};
*/

struct fs_message_create_file{
	enum fs_operation type;
	char * filename;
	int filename_length;
	FH_t dir;
	int code;
};

struct fs_message_lookup{
	enum fs_operation type;
	int code;
	char * name;
	int length;
	FH_t dir;
	FH_t result;
};

struct fs_message_is_directory{
	enum fs_operation type;
	int code;
	FH_t id;
};

struct fs_message_get_path{
	enum fs_operation type;
	int code;
	char * path;
	int length;
	FH_t dir;
};

struct fs_message_create_link{
	enum fs_operation type;
	char * link;
	int length;
	FH_t dir;
	FH_t file;
	int code;
};

struct fs_message_create_dir{
	enum fs_operation type;
	char * name;
	FH_t dir;
	int length;
	int code;
};

struct fs_message_delete{
	enum fs_operation type;
	char * obj;
	int length;
	FH_t dir;
	int code;
};

struct fs_message_read{
	enum fs_operation type;
	FH_t file;
	int count;
	int offset;
	char * buffer;
	int code;
};

struct fs_message_write{
	enum fs_operation type;
	FH_t file;
	int count;
	int offset;
	char * buffer;
	int code;
};

struct fs_message_stat{
	enum fs_operation type;
	FH_t file;
	int code;
};

struct fs_message_stamp{
	enum fs_operation type;
	int code;
	FH_t file;
};

struct fs_message_sync{
	enum fs_operation type;
	int code;
	FH_t file;
};

struct fs_message_readdir{
	enum fs_operation type;
	FH_t dir;
	int offset;
	char * buffer;
	int code;
};

struct fs_message_sync_disk{
	enum fs_operation type;
	int code;
};

struct fs_message_shutdown{
	enum fs_operation type;
	int code;
};

int CreateFile( char * filename, FH_t dir );
int CreateLink( char * link, FH_t dir, FH_t file );
int CreateDir( char * dirname, FH_t dir );
int Delete( char * objname, FH_t dir );
int Read( FH_t file, int count, int offset, char * buffer );
int Write( FH_t file, int count, int offset, char * buffer );
int Stat( FH_t file );
int Stamp( FH_t file );
int Sync( FH_t file );
void SyncDisk(void);
int Shutdown(void);
FH_t Lookup( char * path, FH_t dir );
char * readdir( FH_t dir, int * offset );
int GetPath( char * path, int max_length, FH_t dir );
int IsDirectory( FH_t id );

#endif

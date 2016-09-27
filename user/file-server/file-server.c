#include "fs.h"
#include "yalnix.h"
#include "filesystem.h"
#include "hardware.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* magic bits that determine if a sector contains a file header */
const int MAGIC_HEADER_NUMBER = 0x0badbeef;

/* shutdown will kill set alive to 0 */
static int alive = 1;

/* contains info about file system */
static struct fs_header super_block;

/* directory containing all other directories, including / */
static struct directory root;

/* for disk caching */
struct sector_cache{
	char * data;
	char dirty;
	int block;
	unsigned long long usage;
};

#define MAX_CACHED_SECTORS 32
static struct sector_cache cached[ MAX_CACHED_SECTORS ];

/* keeps track of which blocks are being used */
struct file_block{
	/* 1 or 0 */
	char used;
	/* next block in the list of blocks */
	short next;
};

/* array of file_block, 1 for each sector on disk */
static struct file_block * blocks;

/* keeps track of next inode number */
static unsigned int global_inode_id = ROOTINODE;

/* storage for inodes */
struct inode_list{
	struct inode * node;
	struct inode_list * next;
};

/* storage for directories */
struct directory_list{
	struct directory * dir;
	struct directory_list * next;
};

/* list heads */
static struct inode_list inodes = { .node = 0, .next = 0 };
static struct directory_list directories = { .dir = 0, .next = 0 };

/* get a block and set it to used */
static int findFreeBlock(){
	int block;
	for ( block = NUMBLOCKS - 1; block >= 0; block -= 1 ){
		if ( blocks[ block ].used == 0 ){
			blocks[ block ].used = 1;
			return block;
		}
	}
	return -1;
}

static void freeBlock( int block ){
	blocks[ block ].used = 0;
	blocks[ block ].next = -1;
}

/* returns a cached sector object. if none are available
 * then it finds the one that has been used the least
 * and writes it to disk if the sector is dirty
 */
static struct sector_cache * findCached( int sector ){
	int i;
	for ( i = 0; i < MAX_CACHED_SECTORS; i++ ){
		if ( cached[ i ].block == sector ){
			return &cached[ i ];
		}
		if ( cached[ i ].block == -1 ){
			cached[ i ].usage = 0;
			return &cached[ i ];
		}
	}

	/* reap some old sector */
	struct sector_cache * save = 0;
	unsigned long long minimum = (unsigned long long) -1;
	for ( i = 0; i < MAX_CACHED_SECTORS; i++ ){
		if ( cached[ i ].usage < minimum ){
			minimum = cached[ i ].usage;
			save = &cached[ i ];
		}
	}

	if ( save->dirty ){
		TtyPrintf( 0, "[fs] Syncing sector %d\n", save->block );
		WriteSector( save->block, save->data );
		save->dirty = 0;
	}

	save->usage = 0;

	return save;
}

/* read data from the cache */
static void readCachedSector( int sector, char * data ){
	struct sector_cache * cache = findCached( sector );
	TtyPrintf( 0, "[fs] Read cached sector %d into %p. cache %p\n", sector, data, cache );
	if ( cache->block != sector ){
		TtyPrintf( 0, "[fs] Load sector %d from disk\n", sector );
		ReadSector( sector, cache->data );
		cache->block = sector;
	}
	memcpy( data, cache->data, SECTORSIZE );
	cache->usage += 1;
}

/* write data to the cache */
static void writeCachedSector( int sector, char * data ){
	struct sector_cache * cache = findCached( sector );
	cache->block = sector;
	memcpy( cache->data, data, SECTORSIZE );
	cache->dirty = 1;
	cache->usage += 1;
}

/* return the entry in the directory that has the right name */
struct directory_entry * findEntry( struct directory * root, char * name ){
	int i;
	for ( i = 0; i < root->num_entries; i++ ){
		/*
		if ( root->entries[ i ] ){
			TtyPrintf( 0, "[fs] Compare entry '%s' with '%s'\n", root->entries[ i ]->name, name );
		}
		*/
		// TtyPrintf( 0, "[fs] Check entry [%d] %p in dir [%d] %p\n", i, root->entries[ i ], root->inode_num, root );
		if ( root->entries[ i ] &&
                     strcmp( root->entries[ i ]->name, name ) == 0 ){
			return root->entries[ i ];
		}
	}
	return 0;
}

/* return the entry in the directory with the right id */
struct directory_entry * findEntryById( struct directory * root, int id ){
	int i;
	for ( i = 0; i < root->num_entries; i++ ){
		if ( root->entries[ i ] &&
                     root->entries[ i ]->node->id == id ){
			return root->entries[ i ];
		}
	}
	return 0;
}

/* time for file timestamps */
static unsigned int timeNow(){
	struct timeval t;
	gettimeofday( &t, 0 );
	return t.tv_sec;
}

/* total number of inodes in the system */
static int countInodes(){
	int total = 0;
	struct inode_list * list = inodes.next;
	while ( list != 0 ){
		total += 1;
		list = list->next;
	}

	return total;
}

#if 0
struct directory * lookupDirectory( char * name, struct directory * start ){
	// TtyPrintf( 0, "Lookup in '%s' directory %p\n", name, start );

	if ( start == 0 ){
		return 0;
	}

	char * original = name;
	while ( *name != 0 ){
		char * prev = name;
		while ( *name != '/' && *name != 0 ){
			name++;
		}
		if ( *name == '/' ){
			*name = 0;
			name += 1;
		} else {
			return start;
		}
		int i;
		// TtyPrintf( 0, "[fs] Next dir '%s'. Rest '%s'. current %p\n", prev, name, start );
		/*
		for ( i = 0; i < start->num_entries; i++ ){
			if ( start->entries[ i ] &&
			     strcmp( start->entries[ i ]->name, prev ) == 0 &&
			     start->entries[ i ]->node->type == INODE_DIRECTORY ){
			     	start = start->entries[ i ]->dir;
				i = -1;
				break;
			}
		}
		*/
		struct directory_entry * entry = findEntry( start, prev );

		*(char *)(prev + strlen( prev )) = '/';
		// TtyPrintf( 0, "[fs] Prev now %s\n", prev );

		// if ( i == start->num_entries ){
		if ( entry == 0 ){
			TtyPrintf( 0, "[fs] Didn't find an entry for '%s'\n", original );
			return 0;
		}
		
		if ( entry->node->type != INODE_DIRECTORY ){
			TtyPrintf( 0, "[fs] %s is not a directory\n", original );
			return 0;
		}
		start = entry->dir;
	}

	return start;
}
#endif

/* make a new directory that corresponds to entry. its parent is 'parent' */
static struct directory * createDirectory( struct directory_entry * entry, struct directory * parent ){
	struct directory * dir = (struct directory *) malloc( sizeof( struct directory ) );
	if ( parent->node ){
		dir->parent_id = parent->node->id;
	} else {
		dir->parent_id = 0;
	}
	dir->parent = parent;
	dir->num_entries = 0;
	dir->entries = 0;
	dir->node = entry->node;
	dir->inode_num = entry->inode_num;
	if ( entry ){
		entry->dir = dir;
	}
}

/* search the global directory list for a certain directory */
struct directory * findDirectory( FH_t id ){
	struct directory_list * list = directories.next;
	while ( list != 0 ){
		if ( list->dir->node->id == id ){
			return list->dir;
		}
		list = list->next;
	}
	return 0;
}

/* find the directory that contains some inode */
struct directory * findParent( FH_t id ){
	struct directory_list * list = directories.next;
	while ( list != 0 ){
		if ( findEntryById( list->dir, id ) != 0 ){
			return list->dir;
		}
		list = list->next;
	}
	return 0;
}

/* add a directory to the global directory list */
static struct directory * addDirectory( struct directory * dir ){
	struct directory_list * list = (struct directory_list *) malloc( sizeof( struct directory_list ) );
	list->dir = dir;
	list->next = directories.next;
	directories.next = list;
	return dir;
}

/* erase a directory and remove it from the global directory list */
static void removeDirectory( struct directory * dir ){
	struct directory_list * list = &directories;
	while ( list->next != 0 ){
		if ( list->next->dir == dir ){
			list->next = list->next->next;
			free( dir->entries );
			free( dir );
			break;
		}
		list = list->next;
	}
}

/* returns the number of directory inodes in the system.
 * This is *not* the same as the number of directories in the
 * directory list since multiple directories can point to the
 * same inode.
 */
static int countDirectories(){
	int total = 0;
	struct inode_list * list = inodes.next;
	while ( list != 0 ){
		if ( list->node->type == INODE_DIRECTORY ){
			total += 1;
		}
		list = list->next;
	}

	return total;
}

/* uh hu */
static int isAbsolutePath( char * path ){
	return *path == '/';
}

/* add an inode to the global inode list */
static void addInode( struct inode * node ){
	struct inode_list * list = (struct inode_list *) malloc( sizeof( struct inode_list ) );
	list->node = node;
	list->next = inodes.next;
	inodes.next = list;
}

/* make a new inode */
static struct inode * createInode( int type ){
	struct inode * node = (struct inode *) malloc( sizeof( struct inode ) );
	node->type = type;
	node->nlink = 1;
	node->size = 0;
	node->block = -1;
	node->id = global_inode_id;
	node->timestamp = timeNow();
	global_inode_id += 1;
	addInode( node );
	return node;
}

/* make a file inode */
static struct inode * createInodeFile(){
	return createInode( INODE_REGULAR );
}

/* make a directory inode */
static struct inode * createInodeDir(){
	return createInode( INODE_DIRECTORY );
}

/* find an inode with the proper id */
static struct inode * findInode( unsigned int id ){
	struct inode_list * list = inodes.next;
	while ( list != 0 ){
		if ( list->node->id == id ){
			return list->node;
		}
		list = list->next;
	}
	return 0;
}

/* remove empty space in the array */
static void compactEntries( struct directory * dir ){
	int i;
	int total = 0;
	for ( i = 0; i < dir->num_entries; i++ ){
		if ( dir->entries[ i ] == 0 ){
			int j;
			for ( j = i + 1; j < dir->num_entries; j++ ){
				if ( dir->entries[ j ] ){
					dir->entries[ i ] = dir->entries[ j ];
					dir->entries[ j ] = 0;
					j = dir->num_entries;
					total += 1;
				}
			}
		} else {
			total += 1;
		}
	}

	if ( total < dir->num_entries ){
		struct directory_entry ** entries = (struct directory_entry **) malloc( sizeof( struct directory_entry *) * total );
		memcpy( entries, dir->entries, sizeof( struct directory_entry *) * total );
		free( dir->entries );
		dir->num_entries = total;
		dir->entries = entries;
	}
}

/* returns the entry denoted by path within the directory parent starting from
 * 'dir'.
 * Examples:
 * / returns directory_entry '' parent: root 
 * /foo returns directory entry 'foo' parent: /
 */
static struct directory_entry * splitPath( struct directory ** parent, char * path, FH_t dir_id ){
	struct directory * dir = 0;
	if ( ! isAbsolutePath( path ) ){
		dir = findDirectory( dir_id );
	} else {
		dir = &root;
	}

	if ( dir == 0 ){
		return 0;
	}

	// TtyPrintf( 0, "[fs] Starting dir is %p\n", dir );

	char * start = path;
	struct directory_entry * entry = 0;
	*parent = dir;
	while ( *start != 0 ){
		*parent = dir;
		char * end = strchr( start, '/' );
		int revert = 0;
		if ( end != 0 ){
			*end = 0;
			revert = 1;
		} else {
			end = start + strlen( start );
		}

		// TtyPrintf( 0, "[fs] Split now '%s' start %p end %p. dir %d\n", start, start, end, dir->inode_num );

		entry = findEntry( dir, start );

		if ( revert ){
			*end = '/';
		}

		if ( ! entry ){
			return 0;
		}
		
		// TtyPrintf( 0, "[fs] Found entry %p '%s' in dir %p node %p\n", entry, entry->name, dir, entry->node );

		if ( entry->node->type == INODE_DIRECTORY ){
			dir = entry->dir;
		}

		// TtyPrintf( 0, "[fs] Dir now %p\n", dir );

		start = end;
		while ( *start == '/' ){
			start += 1;
		}
	}

	// TtyPrintf( 0, "[fs] Split found %p %s\n", entry, entry->name );

	return entry;
}

/* return the parent directory for some path
 * /foo/bar/baz - directory for /foo/bar
 * baz does not have to exist.
 */
static struct directory * getParentDirectory( char * path, FH_t dir_id ){
	char * name = strrchr( path, '/' );
	if ( name == 0 ){
		return findDirectory( dir_id );
	}
	char c;
	name += 1;
	c = *name;
	*name = 0;
	// TtyPrintf( 0, "Base dir %s\n", path );
	struct directory * dir;
	struct directory_entry * entry = splitPath( &dir, path, dir_id );
	if ( entry == 0 ){
		dir = 0;
	} else {
		dir = entry->dir;
	}
	*name = c;
	return dir;
}

/* writes out the file header, inodes, directory entries, directories */
static void syncDisk(){
	char data[ SECTORSIZE ];
	TtyPrintf( 0, "[fs] Sync disk\n" );
	super_block.num_inodes = countInodes();
	*(struct fs_header *)data = super_block;
	WriteSector( 1, data );
	int sector = 2;
	int i;

	// TtyPrintf( 0, "[fs] %d inodes. %d directories\n", super_block.num_inodes, countDirectories() );

	char * offset = data;
	struct inode_list * ilist = inodes.next;
	while ( ilist != 0 ){
		*(struct inode *)offset = *(ilist->node);
		offset += sizeof(struct inode);
		if ( offset + sizeof(struct inode) >= data + SECTORSIZE ){
			WriteSector( sector, data );
			sector += 1;
			offset = data;
		}
		ilist = ilist->next;
	}
	if ( offset != data ){
		WriteSector( sector, data );
		sector += 1;
	}
	// TtyPrintf( 0, "[fs] Start writing blocks at sector %d\n", sector );

	offset = data;
	for ( i = 0; i < NUMBLOCKS; i += 1 ){
		// int size = NUMBLOCKS - i < SECTORSIZE ? NUMBLOCKS - i : SECTORSIZE;
		*(struct file_block *) offset = blocks[ i ];
		offset += sizeof( struct file_block );
		if ( offset + sizeof( struct file_block ) >= data + SECTORSIZE ){
			WriteSector( sector, data );
			sector += 1;
			offset = data;
		}
	}
	if ( offset != data ){
		WriteSector( sector, data );
		sector += 1;
	}

	struct directory_list * dlist = directories.next;
	offset = data;
	// TtyPrintf( 0, "[fs] Start writing directories at sector %d\n", sector );
	while ( dlist != 0 ){
		if ( offset + sizeof( struct directory ) >= data + SECTORSIZE ){
			WriteSector( sector, data );
			sector += 1;
			offset = data;
		}
		// TtyPrintf( 0, "[fs] Wrote directory id %d %p offset %p\n", dlist->dir->inode_num, dlist->dir, offset - data  );
		compactEntries( dlist->dir );
		*(struct directory *) offset = *(dlist->dir);
		((struct directory *) offset)->num_entries -= 2;
		offset += sizeof( struct directory );
		int i;
		// TtyPrintf( 0, "[fs] Write %d directory entries at offset %p\n", dlist->dir->num_entries - 2, offset - data );
		for ( i = 0; i < dlist->dir->num_entries; i++ ){
			if ( strcmp( dlist->dir->entries[ i ]->name, "." ) == 0 ||
			     strcmp( dlist->dir->entries[ i ]->name, ".." ) == 0 ){
				continue;
			}
			if ( offset + sizeof(struct directory_entry) >= data + SECTORSIZE ){
				WriteSector( sector, data );
				sector += 1;
				offset = data;
			}
			*(struct directory_entry *) offset = *(dlist->dir->entries[ i ]);
			// TtyPrintf( 0, "[fs] Wrote directory entry id %d name '%s'\n", dlist->dir->entries[ i ]->inode_num, dlist->dir->entries[ i ]->name );
			offset += sizeof( struct directory_entry );
		}

		dlist = dlist->next;
	}
	if ( offset != data ){
		WriteSector( sector, data );
	}
	// TtyPrintf( 0, "[fs] Wrote %d sectors\n", sector );

	for ( i = 0; i < MAX_CACHED_SECTORS; i++ ){
		if ( cached[ i ].dirty ){
			WriteSector( cached[ i ].block, cached[ i ].data );
			cached[ i ].dirty = 0;
		}
	}
}

/* copy a string from some other process */
static char * copyString( int id, char * string, int length ){
	char * path = (char *) malloc( length + 1 );

	if ( CopyFrom( id, path, string, length ) == ERROR ){
		free( path );
		return 0;
	}

	path[ length ] = 0;
	return path;
}

/* make a new directory entry associated with inode 'node' */
static struct directory_entry * createDirectoryEntry( struct inode * node, char * name ){
	struct directory_entry * dir = (struct directory_entry *) malloc( sizeof( struct directory_entry ) );
	dir->node = node;
	dir->inode_num = node->id;
	dir->dir = 0;
	// dir->dir_id = 0;
	strncpy( dir->name, name, DIRNAMELEN );
	return dir;
}

/* add a directory entry to a directory */
static struct directory_entry * addEntry( struct directory * dir, struct inode * node, char * name ){
	struct directory_entry * entry = createDirectoryEntry( node, name );
	int index = 0;
	for ( index = 0; index < dir->num_entries && dir->entries[ index ] != 0; index++ ){
		/**/
	}

	if ( index == dir->num_entries ){
		int size = dir->num_entries * 2 + 1;
		struct directory_entry ** entries = (struct directory_entry **) malloc( sizeof(struct directory_entry * ) * size );
		bzero( entries, size * sizeof(struct directory_entry *) );
		memcpy( entries, dir->entries, dir->num_entries * sizeof(struct directory_entry *) );
		dir->num_entries = size;
		free( dir->entries );
		dir->entries = entries;
	}

	dir->entries[ index ] = entry;
	dir->node->timestamp = timeNow();
	return entry;
}

/* make a new file in the filesystem. add it to the directory given
 * by the message.
 */
static void handleCreateFile( int id, struct fs_message_create_file * message ){
	char * name = copyString( id, message->filename, message->filename_length );

	struct directory * dir = getParentDirectory( name, message->dir );

	if ( dir == 0 ){
		message->code = ERROR;
	} else {
		char * filename = strrchr( name, '/' );
		if ( filename != 0 ){
			filename += 1;
		} else {
			filename = name;
		}

		if ( findEntry( dir, filename ) != 0 ){
			message->code = ERROR;
		} else {
			struct inode * node = createInodeFile();
			addEntry( dir, node, filename );
			message->code = 0;
		}
	}

	free( name );
}

/* shutdown the server */
static void handleShutdown( int id, struct fs_message_shutdown * message ){
	alive = 0;
	TtyPrintf( 0, "[fs] Shutdown\n" );
	message->code = 0;
	syncDisk();
}

/* make a hard link to a file or directory, which basically
 * means incrementing the nlink field of an inode and making
 * a new entry in some directory.
 */
static void handleLink( int id, struct fs_message_create_link * message ){
	char * path = copyString( id, message->link, message->length );

	FH_t dir_id = message->dir;
	FH_t link_id = message->file;

	struct directory * dir = findDirectory( dir_id );
	struct inode * node = findInode( link_id );

	if ( dir == 0 || node == 0 || findEntry( dir, path ) != 0 ){
		message->code = ERROR;
	} else {
		TtyPrintf( 0, "[fs] Create link %s in dir %d. link_id %d node %p\n", path, dir->node->id, link_id, node );
		node->nlink += 1;
		if ( node->type == INODE_REGULAR ){
			TtyPrintf( 0, "[fs] Linked file '%s' in %s\n", path, findEntryById( dir->parent, dir->node->id )->name );
			addEntry( dir, node, path );
			message->code = 0;
		} else if ( node->type == INODE_DIRECTORY ){
			TtyPrintf( 0, "[fs] Linked directory '%s' in %s\n", path, findEntryById( dir->parent, dir->node->id )->name );
			struct directory * link_dir = findDirectory( node->id );
			struct directory_entry * entry = addEntry( dir, node, path );
			entry->dir = link_dir;
			message->code = 0;
		}
	}

	free( path );
}

/* read an entry from a directory */
static void handleReadDir( int id, struct fs_message_readdir * message ){

	struct directory * dir = findDirectory( message->dir );
	// TtyPrintf( 0, "[fs] Found directory %p with id %d\n", dir, message->dir );
	if ( ! dir ){
		message->code = 0;
	} else {
		int entry = message->offset;
		while ( entry < dir->num_entries && dir->entries[ entry ] == 0 ){
			entry += 1;
		}
		if ( entry >= dir->num_entries ){
			// TtyPrintf( 0, "[fs] No such entry for %d\n", message->offset );
			message->code = 0;
		} else {
			message->offset = entry + 1;
			// TtyPrintf( 0, "[fs] Found file %s\n", dir->entries[ entry ]->name );
			CopyTo( id, message->buffer, dir->entries[ entry ]->name, DIRNAMELEN );
			message->code = 1;
		}
	}
}

/* make a new directory */
static void handleCreateDir( int id, struct fs_message_create_dir * message ){
	char * path = copyString( id, message->name, message->length );

	struct directory * dir = getParentDirectory( path, message->dir );

	char * name = strrchr( path, '/' );
	if ( name == 0 ){
		name = path;
	} else {
		name += 1;
	}

	TtyPrintf( 0, "[fs] Attempting to create '%s' in dir %p\n", name, dir );
	if ( dir == 0 || findEntry( dir, name ) != 0 ){
		message->code = ERROR;
	} else {
		message->code = 0;

		struct directory_entry * entry = addEntry( dir, createInodeDir(), name );
		struct directory * d = addDirectory( createDirectory( entry, dir ) );
		struct directory_entry * d__ = addEntry( d, dir->node, ".." );
		d__->dir = dir;
		struct directory_entry * d_ = addEntry( d, d->node, "." );
		d_->dir = d;
		TtyPrintf( 0, "[fs] Created directory %p\n", d );
	}

	free( path );
}

/* count the number of alive entries in a directory */
static int countEntries( struct directory * dir ){
	int count = 0;
	int i;
	for ( i = 0; i < dir->num_entries; i++ ){
		if ( dir->entries[ i ] ){
			count += 1;
		}
	}
	return count;
}

/* find the id of the inode for the given path */
static void handleLookup( int id, struct fs_message_lookup * message ){
	char * path = copyString( id, message->name, message->length );

	// TtyPrintf( 0, "[fs] Lookup %s\n", path );

	if ( path == 0 ){
		message->code = ERROR;
		return;
	}

	struct directory * parent;
	struct directory_entry * entry = splitPath( &parent, path, message->dir );

	// TtyPrintf( 0, "[fs] Found entry %p in dir %p\n", entry, parent );

	if ( entry ){
		message->result = entry->node->id;
	} else {
		message->result = ERROR;
	}

	free( path );
}

/* stop using some cached sector */
static void releaseCachedBlock( int block ){
	int i;
	for ( i = 0; i < MAX_CACHED_SECTORS; i++ ){
		if ( cached[ i ].block == block ){
			cached[ i ].block = -1;
		}
	}
}

/* release all cached sectors used by an inode */
static void releaseCachedBlocks( struct inode * node ){
	int block = node->block;
	while ( block != -1 ){
		releaseCachedBlock( block );
		block = blocks[ block ].next;
	}
}

/* remove an inode from the system. free all its resources */
static void eraseInode( struct inode * node ){
	struct inode_list * list = &inodes;
	while ( list->next != 0 ){
		if ( list->next->node == node ){
			list->next = list->next->next;
			break;
		}
		list = list->next;
	}

	releaseCachedBlocks( node );
	free( node );
}

/* remove an entry from a directory */
static void removeEntry( struct directory * parent, struct directory_entry * entry ){
	int i;
	for ( i = 0; i < parent->num_entries; i++ ){
		if ( parent->entries[ i ] == entry ){
			TtyPrintf( 0, "[fs] Removed %s from %p\n", entry->name, parent );
			parent->entries[ i ] = 0;
			free( entry );
			break;
		}
	}
}

/* delete a file/directory, or just reduce its nlink count if above 1 */
static void handleDelete( int id, struct fs_message_delete * message ){
	char * path = copyString( id, message->obj, message->length );

	if ( path == 0 ){
		message->code = ERROR;
		return;
	}

	struct directory * parent;
	struct directory_entry * entry = splitPath( &parent, path, message->dir );

	if ( entry ){
		if ( entry->node->type == INODE_REGULAR ){
			entry->node->nlink -= 1;
			if ( entry->node->nlink == 0 ){
				TtyPrintf( 0, "[fs] Delete file '%s'\n", entry->name );
				eraseInode( entry->node );
			}
			removeEntry( parent, entry );
		} else if ( !(strcmp( entry->name, "." ) == 0 ||
		              strcmp( entry->name, ".." ) == 0 ) && 
			      (entry->node->nlink > 1 || countEntries( entry->dir ) == 2) ){
			TtyPrintf( 0, "[fs] Delete directory '%s' %p. parent '%s' %p\n", entry->name, entry, findEntryById( parent->parent, parent->node->id )->name, parent );
			entry->node->nlink -= 1;
			if ( entry->node->nlink == 0 ){
				eraseInode( entry->node );
				removeDirectory( entry->dir );
			}
			removeEntry( parent, entry );
		} else {
			TtyPrintf( 0, "[fs] Directory '%s' is not empty. Cannot delete.\n", entry->name );
		}
		message->code = 0;
	} else {
		message->code = ERROR;
	}

	free( path );
}

/* store the full path for some directory in buffer */
static void getLineage( char ** buffer, struct directory * dir, char * start, int max ){
	// TtyPrintf( 0, "[fs] Get lineage for dir %p. parent %p. node %p\n", dir, dir->parent, dir->node );
	// TtyPrintf( 0, "[fs] Entry is %p\n", findEntryById( dir->parent, dir->node->id ) );
	char * name = findEntryById( dir->parent, dir->node->id )->name;
	if ( *buffer + strlen( name ) >= start + max ){
		return;
	}

	if ( dir->parent->parent != 0 ){
		getLineage( buffer, dir->parent, start, max );
	}

	char * n = *buffer;
	memcpy( *buffer, name, strlen( name ) );
	*buffer += strlen( name );
	**buffer = '/';
	*buffer += 1;
	**buffer = 0;

	/*
	strcat( *buffer, name );
	strcat( *buffer, "/" );
	*/
}

/* return the full path for some inode */
static void handleGetPath( int id, struct fs_message_get_path * message ){

	struct inode * node = findInode( message->dir );
	if ( node == 0 ){
		message->code = ERROR;
	} else {
		message->code = 0;
		if ( node->type == INODE_DIRECTORY ){
			// TtyPrintf( 0, "[fs] Find directory for node %p. id %d\n", node, node->id );
			struct directory * dir = findDirectory( node->id );
			char buffer[ 1024 ];
			char * offset = buffer;
			getLineage( &offset, dir, buffer, 1024 );
			// TtyPrintf( 0, "[fs] Got lineage '%s'\n", buffer );
			CopyTo( id, message->path, buffer, message->length > 1024 ? 1024 : message->length );
		} else {
			char buffer[ 1024 ];
			struct directory * dir = findParent( node->id );
			char * offset = buffer;
			getLineage( &offset, dir, buffer, 1024 );
			strcat( buffer, findEntryById( dir, node->id )->name );
			CopyTo( id, message->path, buffer, message->length > 1024 ? 1024 : message->length );
		}
	}
}

/* the maximum size an inode can be by adding up all the
 * sector space it owns.
 */
static int maxInodeSize( struct inode * node ){
	int total = 0;

	int block = node->block;
	while ( block != -1 ){
		total += SECTORSIZE;
		block = blocks[ block ].next;
	}

	return total;
}

/* read some bytes from an inode */
static void handleRead( int id, struct fs_message_read * message ){
	struct inode * node = findInode( message->file );
	int bytes = message->count;
	int offset = message->offset;
	char * user_buffer = message->buffer;
	int max_offset = bytes + offset;

	if ( node == 0 || node->type == INODE_DIRECTORY ){
		message->code = ERROR;
		return;
	}
	
	TtyPrintf( 0, "[fs] Reading %d bytes from file %d at offset %d\n", bytes, node->id, offset );

	int block = node->block;
	if ( block == -1 ){
		message->code = ERROR;
		return;
	}
	
	bytes = offset + bytes > node->size ? node->size - offset : bytes;

	while ( offset >= SECTORSIZE ){
		offset -= SECTORSIZE;
		block = blocks[ block ].next;
		if ( block == -1 ){
			message->code = ERROR;
			return;
		}
	}

	char data[ SECTORSIZE ];
	// ReadSector( block, data );
	readCachedSector( block, data );
	int max_length = SECTORSIZE - offset;
	int bytes_read = 0;
	while ( bytes > 0 ){
		int min = max_length < bytes ? max_length : bytes;
		TtyPrintf( 0, "[fs] Reading %d bytes at offset %d from block %d\n", min, offset, block );
		CopyTo( id, user_buffer, data + offset, min );
		max_length -= min;
		bytes -= min;
		bytes_read += min;
		if ( max_length == 0 && bytes > 0 ){
			block = blocks[ block ].next;
			readCachedSector( block, data );
			max_length = SECTORSIZE;
			offset = 0;
		}
	}

	message->code = bytes_read;
}

/* write some bytes to an inode */
static void handleWrite( int id, struct fs_message_write * message ){

	struct inode * node = findInode( message->file );
	int bytes = message->count;
	int offset = message->offset;
	char * user_buffer = message->buffer;
	int max_offset = bytes + offset;

	if ( node == 0 || node->type == INODE_DIRECTORY ){
		message->code = ERROR;
		return;
	}

	TtyPrintf( 0, "[fs] Writing %d bytes into file %d at offset %d\n", bytes, node->id, offset );

	if ( node->block == -1 ){
		node->block = findFreeBlock();
		if ( node->block == -1 ){
			message->code = ERROR;
			return;
		}
	}

	int block = node->block;
	while ( blocks[ block ].next != -1 ){
		block = blocks[ block ].next;
	}

	/* make sure file is big enough */
	while ( offset + bytes > maxInodeSize( node ) ){
		int next_block = findFreeBlock();
		if ( next_block == -1 ){
			message->code = ERROR;
			return;
		}
		blocks[ block ].next = next_block;
		block = next_block;
	}

	/* seek to the write block */
	block = node->block;
	while ( offset >= SECTORSIZE ){
		block = blocks[ block ].next;
		offset -= SECTORSIZE;
	}

	node->timestamp = timeNow();

	TtyPrintf( 0, "[fs] Start to write at offset %d in block %d\n", offset, block );

	char data[ SECTORSIZE ];
	// ReadSector( block, data );
	readCachedSector( block, data );
	int max_length = SECTORSIZE - offset;
	while ( bytes > 0 ){
		int min = max_length < bytes ? max_length : bytes;
		TtyPrintf( 0, "[fs] Writing %d bytes at offset %d into block %d\n", min, offset, block );
		CopyFrom( id, data + offset, user_buffer, min );
		// WriteSector( block, data );
		writeCachedSector( block, data );
		max_length -= min;
		bytes -= min;
		if ( max_length == 0 && bytes > 0 ){
			block = blocks[ block ].next;
			// ReadSector( block, data );
			readCachedSector( block, data );
			max_length = SECTORSIZE;
			offset = 0;
		}
	}

	TtyPrintf( 0, "[fs] Wrote %d bytes in file %d\n", message->count, node->id );

	node->size = node->size < max_offset ? max_offset : node->size;

	message->code = message->count;
}

/* write out the file system information */
static void handleSyncDisk( int id, struct fs_message_sync_disk * message ){
	syncDisk();
}

/* return the size of an inode */
static void handleStat( int id, struct fs_message_stat * message ){
	struct inode * node = findInode( message->file );
	if ( node == 0 ){
		message->code = ERROR;
	} else {
		message->code = node->size;
	}
}

/* 1 if an inode is a directory, 0 otherwise */
static void handleIsDirectory( int id, struct fs_message_is_directory * message ){
	struct directory * dir = findDirectory( message->id );
	message->code = dir != 0;
}

/* return the last modification time of an inode */
static void handleStamp( int id, struct fs_message_stamp * message ){
	struct inode * node = findInode( message->file );
	if ( node == 0 ){
		message->code = ERROR;
	} else {
		message->code = node->timestamp;
	}
}

/* write a cached sector to disk */
static void syncCachedSector( int block ){
	int i;
	for ( i = 0; i < MAX_CACHED_SECTORS; i++ ){
		if ( cached[ i ].block == block && cached[ i ].dirty ){
			TtyPrintf( 0, "[fs] Syncing sector %d\n", cached[ i ].block );
			WriteSector( cached[ i ].block, cached[ i ].data );
			cached[ i ].dirty = 0;
		}
	}
}

/* write all sectors used by this inode to disk if they are dirty */
static void forceSync( struct inode * node ){
	int block = node->block;
	while ( block != -1 ){
		syncCachedSector( block );
		block = blocks[ block ].next;
	}
}

/* invoke the above synchronization methods for sectors */
static void handleSync( int id, struct fs_message_sync * message ){
	struct inode * node = findInode( message->file );

	if ( ! node || node->type == INODE_DIRECTORY ){
		message->code = ERROR;
	} else {
		forceSync( node );
	}
}

/* polymorphic function converter.
 * converts unknown data type to some concrete type
 */
void handle( int id, void * data ){
	enum fs_operation type = *(enum fs_operation *)data;
	switch( type ){
		default : {
			TtyPrintf( 0, "[fs] No operation specified\n" );
			break;
		}
		case NONE : {
			TtyPrintf( 0, "[fs] No operation specified\n" );
			break;
		}
		case CREATE_FILE : {
			handleCreateFile( id, (struct fs_message_create_file *) data );
			break;
		}
		case SHUTDOWN : {
			handleShutdown( id, (struct fs_message_shutdown *) data );
			break;
		}
		case CREATE_LINK : {
			handleLink( id, (struct fs_message_create_link *) data );
			break;
		}
		case LOOKUP : {
			handleLookup( id, (struct fs_message_lookup *) data );
			break;
		}
		case CREATE_DIR : {
			handleCreateDir( id, (struct fs_message_create_dir *) data );
			break;
		}
		case DELETE : {
			handleDelete( id, (struct fs_message_delete *) data );
			break;
		}
		case IS_DIRECTORY : {
			handleIsDirectory( id, (struct fs_message_is_directory *) data );
			break;
		}
		case READ : {
			handleRead( id, (struct fs_message_read *) data );
			break;
		}
		case WRITE : {
			handleWrite( id, (struct fs_message_write *) data );
			break;
		}
		case STAT : {
			handleStat( id, (struct fs_message_stat *) data );
			break;
		}
		case STAMP : {
			handleStamp( id, (struct fs_message_stamp *) data );
			break;
		}
		case SYNC : {
			handleSync( id, (struct fs_message_sync *) data );
			break;
		}
		case GET_PATH : {
			handleGetPath( id, (struct fs_message_get_path *) data );
			break;
		}
		case SYNC_DISK : {
			handleSyncDisk( id, (struct fs_message_sync_disk *) data );
			break;
		}
		case READDIR : {
			handleReadDir( id, (struct fs_message_readdir *) data );
			break;
		}
	}
}

/* create links from directory entries to directories */
static void populateDirectories( struct directory * root, struct directory ** all, int num ){
	int i;
	
	for ( i = 0; i < root->num_entries; i++ ){
		if ( root->entries[ i ] ){
			int j;
			for ( j = 0; j < num; j++ ){
				if ( all[ j ] ){
					// TtyPrintf( 0, "[fs] Check [%d] inode_num %d. root id %d\n", j, all[ j ]->inode_num, root->entries[ i ]->inode_num );
				}
				if ( all[ j ] && all[ j ]->inode_num == root->entries[ i ]->inode_num && all[ j ]->parent_id == root->inode_num ){
					// TtyPrintf( 0, "Populate '%s'\n", root->entries[ i ]->name );
					root->entries[ i ]->dir = all[ j ];
					all[ j ]->parent = root;
					if ( root->node ){
						struct directory_entry * d__ = addEntry( all[ j ], root->node, ".." );
						d__->dir = root;
					} else {
						struct directory_entry * d__ = addEntry( all[ j ], all[ j ]->node, ".." );
						d__->dir = all[ j ];
					}
					populateDirectories( all[ j ], all, num );
				}
			}
		}
	}
}

/* read file system information from sector 1 or start a new file system */
static void initializeSuperBlock( struct fs_header * block ){
	char data[ SECTORSIZE ];
	ReadSector( 1, data );
	*block = *(struct fs_header *)data;
	TtyPrintf( 0, "[fs] Initialize super block\n" );
	blocks = (struct file_block *) malloc( sizeof( struct file_block ) * NUMBLOCKS );
	int i;
	for ( i = 0; i < NUMBLOCKS; i++ ){
		freeBlock( i );
	}
	for ( i = 0; i < MAX_CACHED_SECTORS; i++ ){
		char * d = (char *) malloc( SECTORSIZE );

		if ( ! d ){
			TtyPrintf( 0, "[fs] Out of memory\n" );
			Exit( -1 );
		}

		cached[ i ] = (struct sector_cache){ .block = -1, .data = d, .usage = 0, .dirty = 0 };
	}

	if ( block->magic != MAGIC_HEADER_NUMBER ){
		TtyPrintf( 0, "[fs] Create new super block\n" );
		block->magic = MAGIC_HEADER_NUMBER;
		block->num_blocks = 0;
		block->num_inodes = 1;

		struct inode * r = createInodeDir();
		struct directory_entry * entry = createDirectoryEntry( r, "" );
		struct directory_entry ** entries = (struct directory_entry **) malloc( sizeof( struct directory_entry *) * 1 );
		entries[ 0 ] = entry;
		root = (struct directory){ .parent_id = 0, .node = 0, .num_entries = 1, .entries = entries };
		struct directory * dir = createDirectory( entry, &root );
		addDirectory( dir );
		struct directory_entry * d__ = addEntry( dir, dir->node, "." );
		d__->dir = dir;
		struct directory_entry * d_ = addEntry( dir, dir->node, ".." );
		d_->dir = dir;
	} else {
		TtyPrintf( 0, "[fs] Read super block from disk\n" );
		int inode_size = block->num_inodes;
		int i;
		int sector = 2;
		// char * buffer = (char *) malloc( SECTORSIZE );
		char * buffer = data;
		ReadSector( sector, buffer );

		// TtyPrintf( 0, "[fs] Reading %d inodes\n", inode_size );

		char * offset = buffer;
		for ( i = 0; i < inode_size; i++ ){
			struct inode * node = createInodeFile( 0 );
			*node = *(struct inode *)offset;
			if ( node->id >= global_inode_id ){
				global_inode_id = node->id + 1;
			}
			offset += sizeof(struct inode);
			if ( offset + sizeof(struct inode) > buffer + SECTORSIZE ){
				sector += 1;
				ReadSector( sector, buffer );
				offset = buffer;
			}
		}
		if ( offset != buffer ){
			sector += 1;
			ReadSector( sector, data );
		}

		// free( buffer );
		// TtyPrintf( 0, "[fs] Start reading blocks at sector %d\n", sector );

		offset = data;
		for ( i = 0; i < NUMBLOCKS; i += 1 ){
			blocks[ i ] = *(struct file_block *) offset;
			offset += sizeof( struct file_block );
			if ( offset + sizeof( struct file_block ) >= data + SECTORSIZE ){
				sector += 1;
				ReadSector( sector, data );
				offset = data;
			}
		}
		if ( offset != buffer ){
			sector += 1;
		}

		int dirs = countDirectories();

		// TtyPrintf( 0, "[fs] %d directories to read\n", dirs );

		struct directory ** all_dirs = (struct directory **) malloc( sizeof( struct directory ) * dirs );

		// buffer = (char *) malloc( SECTORSIZE );
		// TtyPrintf( 0, "[fs] Start reading directories at sector %d\n", sector );
		buffer = data;
		ReadSector( sector, buffer );
		// char * offset = buffer;
		offset = buffer;
		for ( i = 0; i < dirs; i++ ){
			if ( offset + sizeof(struct directory) >= buffer + SECTORSIZE ){
				sector += 1;
				ReadSector( sector, buffer );
				offset = buffer;
			}
			// TtyPrintf( 0, "[fs] Reading directory %d at offset %p\n", i + 1, offset - data );

			struct directory * dir = (struct directory *) malloc( sizeof( struct directory ) );
			*dir = *(struct directory *) offset;
			// TtyPrintf( 0, "[fs] Read directory with id %d\n", dir->inode_num );
			dir->node = findInode( dir->inode_num );
			// dir->entries = (struct directory_entry **) malloc( sizeof( struct directory_entry *) * dir->num_entries );
			dir->entries = 0;
			if ( dir->num_entries > 0 ){
				dir->entries = (struct directory_entry **) malloc( sizeof( struct directory_entry *) * dir->num_entries );
				bzero( dir->entries, sizeof( struct directory_entry *) * dir->num_entries );
			}
			offset += sizeof( struct directory );
			int j;
			// TtyPrintf( 0, "[fs] Read %d directory entries at offset %p\n", dir->num_entries, offset - data );
			for ( j = 0; j < dir->num_entries; j++ ){
				if ( offset + sizeof(struct directory_entry ) >= buffer + SECTORSIZE ){
					sector += 1;
					ReadSector( sector, buffer );
					offset = buffer;
				}
				struct directory_entry * entry = (struct directory_entry *) malloc( sizeof( struct directory_entry ) );
				*entry = *(struct directory_entry *) offset;
				// TtyPrintf( 0, "[fs] Read entry [%d] '%s'\n", j, entry->name );
				offset += sizeof( struct directory_entry );
				entry->node = findInode( entry->inode_num );
				// TtyPrintf( 0, "[fs] Directory entry %p id %d. name '%s'\n", entry, entry->inode_num, entry->name );
				// entry->inode_num = entry->node->id;
				dir->entries[ j ] = entry;
			}
			struct directory_entry * d_ = addEntry( dir, dir->node, "." );
			d_->dir = dir;
			all_dirs[ i ] = dir;
			addDirectory( dir );
			// TtyPrintf( 0, "[fs] Directory index %d\n", i );
		}
		// free( buffer );

		struct inode * r = findInode( ROOTINODE );
		struct directory_entry * entry = createDirectoryEntry( r, "" );
		struct directory_entry ** entries = (struct directory_entry **) malloc( sizeof( struct directory_entry *) * 1 );
		entries[ 0 ] = entry;
		
		// TtyPrintf( 0, "[fs] Populate directories\n" );
		root = (struct directory){ .parent_id = 0, .inode_num = 0, .num_entries = 1, .entries = entries };
		populateDirectories( &root, all_dirs, dirs );
		free( all_dirs );

		// TtyPrintf( 0, "[fs] Read disk header\n" );
	}

	/* just make the first 10 blocks non-useable */
	for ( i = 0; i < 10; i++ ){
		blocks[ i ].used = 1;
	}
}

int main(){

	if ( Register( FILE_SERVER ) == ERROR ){
		TtyPrintf( 0, "[fs] Could not register file server port %d\n", FILE_SERVER );
		return -1;
	}

	initializeSuperBlock( &super_block );

	char message[ 32 ];
	int from;
	while ( alive && (from = Receive( message )) != ERROR ){
		handle( from, message );
		Reply( message, from );
	}
	
	TtyPrintf( 0, "[fs] Shutdown success\n" );

	return 0;
}

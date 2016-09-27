#ifndef _yalnix_memory_h
#define _yalnix_memory_h

#define region0_memory (VMEM_0_LIMIT - VMEM_0_BASE)
#define region0_pages (region0_memory / PAGESIZE)
#define region1_pages (VMEM_1_SIZE / PAGESIZE)

#include "debug.h"

/* if you want to copy memory from this page use virtual, not physical */
struct memory_page{
	/* physical frame number */
	int frame;
	/* virtual frame number */
	int virtual;

	/* protection flags
	 * PROT_READ
	 * PROT_WRITE
	 * PROT_EXEC
	 */
	int protection;

	/* linked list pointer to next page when this page is stored in
	 * a list of pages( free/used )
	 */
	struct memory_page * next;
};

int findUnusedPage0();
int mapUnusedPage0( int frame, int virtual );
void unMapPage0( int virtual );

void copyPage( int page1, int page2 );
void flushTLB0();
void flushTLB1();
struct memory_page * get_free_page();
void add_free_page( struct memory_page * page );
int to_page( unsigned long addr, int distance );
void setupKernelStack( struct memory_page ** stack, int num_pages );
void setupPageTable( struct memory_page ** table, int size );
YalnixError copyBlockToPage( struct memory_page * page, char * block, int size );
int freePages( int i );

void initializeVirtualMemory( const unsigned max_memory );
void modifyPageTable1( int index, char valid, char mode, int frame );

int user_brk( struct memory_page ** pages, int heap_start, int heap_end, void * memory );
int grow_stack( struct memory_page ** pages, int start );

int ensureReadInTable( struct memory_page ** table, void * p, int size );
int ensureReadWriteInTable( struct memory_page ** table, void * p, int size );
int ensureRegion1Read( void * p, int size );
int ensureRegion1ReadWrite( void * p, int size );
int ensureRegion1StringRead( char * p );
int ensureRegion1StringReadWrite( char * p );

#endif

#include "yalnix.h"
#include "hardware.h"
#include "memory.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct pte region0_page_table[ region0_pages ];
static struct pte region1_page_table[ region1_pages ];

static void * top_of_kernel_heap = 0;
static void * bottom_of_kernel_heap = 0;
static int virtual_memory_enabled = 0;

/* flush TLB for region 0 */
void flushTLB0(){
	WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_0 );
}

/* flush every address for page virtual */
void flushTLB0Page( int virtual ){
	int i = 0;
	for ( i = virtual << PAGESHIFT; i < (virtual << PAGESHIFT) + PAGESIZE; i++ ){
		WriteRegister( REG_TLB_FLUSH, i );
	}
}
	
/* flush TLB for region 1 */
void flushTLB1(){
	WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_1 );
}

/* free list of pages */
static struct memory_page memory_page_free = { .frame = 0, .next = 0 };
/* used list of pages */
static struct memory_page memory_page_used = { .frame = 0, .next = 0 };

/* update an array of pte structures */
static void modifyPageTable( struct pte * table, int index, char valid, char mode, int frame ){
	table[ index ] = (struct pte){ .valid = valid, .prot = mode, .pfn = frame };
}

/* update region 0 */
static void modifyPageTable0( int index, char valid, char mode, int frame ){
	modifyPageTable( region0_page_table, index, valid, mode, frame );
}

/* update region 1 */
void modifyPageTable1( int index, char valid, char mode, int frame ){
	modifyPageTable( region1_page_table, index, valid, mode, frame );
}

/* 1 if index is invalid in the array table, 0 otherwise */
static int invalidPage( struct pte * table, int index ){
	return table[ index ].valid == 0;
}

/* invalid page in region 0 */
static int invalidPage0( int index ){
	return invalidPage( region0_page_table, index );
}

/* invalid page in region 1 */
static int invalidPage1( int index ){
	return invalidPage( region1_page_table, index );
}

/* convert addr to a page frame number and add distance to it */
int to_page( unsigned long addr, int distance ){
	return (addr >> PAGESHIFT) + distance;
}

/* copy a block from virtual page2 to virtual page1
 * page1 - dest
 * page2 - src
 * bytes copied = PAGESIZE
 */
void copyPage( int page1, int page2 ){
	memcpy( (void *)(page1 << PAGESHIFT), (void *)(page2 << PAGESHIFT), PAGESIZE );
}

/* returns an unused page in region 0 or -1 if there are none left */
int findUnusedPage0(){
	int i = 0;
	for ( i = 0; i < region0_pages; i++ ){
		if ( invalidPage0( i ) ){
			return i;
		}
	}

	return -1;
}

/* map a page to some unused physical page and return the virtual frame number in region 0 */
int mapUnusedPage0( int frame, int virtual ){
	if ( virtual == -1 ){
		virtual = findUnusedPage0();
	}

	if ( virtual == -1 ){
		return -1;
	}

	modifyPageTable0( virtual, 1, PROT_READ | PROT_WRITE, frame );

	/* could just unmap virtual to virtual + PAGESIZE instead of all of region 0.. */
	flushTLB0Page( virtual );

	return virtual;
}

void unMapPage0( int virtual ){
	modifyPageTable0( virtual, 0, PROT_READ | PROT_WRITE, virtual );
	flushTLB0Page( virtual );
}

static int isVirtualMemoryEnabled(){
	return virtual_memory_enabled;
}

static int checkRegion1( void * data, int check, int protection ){
	struct pte * table = (struct pte *) data;
	if ( check < 0 || check >= region1_pages ){
		return 0;
	}
	if ( ! table[ check ].valid ){
		return 0;
	}

	if ( (table[ check ].prot & protection) != protection ){
		return 0;
	}
	return 1;
}

static int ensure( void * p, int size, int protection, void * data, int (*check)(void*,int,int) ){
	/*
	int i = 0;
	int base = to_page( VMEM_1_BASE, 0 );
	for ( i = 0; i < size; i += PAGESIZE ){
		int check = to_page( (unsigned int)(p + i), 0 ) - base;
		TracePrintf( 12, "Ensure region 1 %p %d\n", p + i, check );
		if ( ! checkRegion1( check, protection ) ){
			return 0;
		}
	}
	if ( ! checkRegion1( to_page( (unsigned int)(p + size - 1), 0 ) - base, protection ) ){
		return 0;
	}
	*/

	int i = 0;
	int base = to_page( VMEM_1_BASE, 0 );
	for ( i = 0; i < size; i += PAGESIZE ){
		int page = to_page( (unsigned int)(p + i), 0 ) - base;
		TracePrintf( 12, "Ensure region 1 %p %d\n", p + i, page );
		if ( ! check( data, page, protection ) ){
			return 0;
		}
	}
	if ( ! check( data, to_page( (unsigned int)(p + size - 1), 0 ) - base, protection ) ){
		return 0;
	}

	return 1;
}

static int checkTable( void * data, int check, int protection ){
	struct memory_page ** table = (struct memory_page **) data;
	if ( check < 0 || check >= region1_pages ){
		return 0;
	}

	if ( ! table[ check ] ){
		return 0;
	}

	if ( (table[ check ]->protection & protection) != protection ){
		return 0;
	}

	return 1;
}

/* 1 if [p, p+size) is inside region 1, 0 otherwise */
static int ensureRegion1( void * p, int size, int protection ){
	return ensure( p, size, protection, (void *) &region1_page_table, checkRegion1 );	
}

int ensureReadInTable( struct memory_page ** table, void * p, int size ){
	return ensure( p, size, PROT_READ, table, checkTable );
}

int ensureReadWriteInTable( struct memory_page ** table, void * p, int size ){
	return ensure( p, size, PROT_WRITE | PROT_READ, table, checkTable );
}

int ensureRegion1Read( void * p, int size ){
	return ensureRegion1( p, size, PROT_READ );
}

int ensureRegion1ReadWrite( void * p, int size ){
	return ensureRegion1( p, size, PROT_READ | PROT_WRITE );
}

/* 1 if every byte in p is in region 1, 0 otherwise
 * you can pass a string without a null value and this routine will
 * just run off until it hits VMEM_1_LIMIT.
 */
int ensureRegion1String( char * p, int protection ){
	while ( *p ){
		if ( ! ensureRegion1( p, sizeof(char), protection ) ){
			return 0;
		}
		p += 1;
	}
	return 1;
}

int ensureRegion1StringRead( char * p ){
	return ensureRegion1String( p, PROT_READ );
}

int ensureRegion1StringReadWrite( char * p ){
	return ensureRegion1String( p, PROT_READ | PROT_WRITE );
}

/*
static int countPages( struct memory_page * page ){
	int count = 0;
	page = page->next;
	while ( page != 0 ){
		count += 1;
		page = page->next;
	}
	return count;
}
*/

/* returns a free *physical* page from the free list of memory */
struct memory_page * get_free_page(){
	struct memory_page * page;

	if ( memory_page_free.next != 0 ){
		page = memory_page_free.next;	
		memory_page_free.next = memory_page_free.next->next;
		TracePrintf( 10, "Use free page %d\n", page->frame );
	} else {
		TracePrintf( 10, "No more free physical pages\n" );
		return 0;
	}
	
	page->next = memory_page_used.next;
	memory_page_used.next = page;

	// TracePrintf( 5, "Free pages left %d\n", countPages( &memory_page_free ) );

	return page;
}

/* add a physical page of memory to the free list */
void add_free_page( struct memory_page * page ){
	// printf( "Add page %p frame %d\n", page, page->frame );
	TracePrintf( 10, "Add free page %d\n", page->frame );
	page->next = memory_page_free.next;
	memory_page_free.next = page;
}

/* returns 1 if there are at least i pages of memory available */
int freePages( int i ){
	int count = 0;
	struct memory_page * page = memory_page_free.next;
	while ( count < i && page != 0 ){
		count += 1;
		page = page->next;
	}
	return count >= i;
}

/* map all pages starting from 'start' to vmem_1_limit
 * pages aren't in the TLB but the TLB doesn't have to be flushed
 * since region 1 is flushed when this process is mapped in anyway
 * so if a new address is used that the TLB hasn't seen then it will
 * be cached as normal.
 */
int grow_stack( struct memory_page ** table, int start ){
	int i;
	TracePrintf( 4, "Grow stack in region 1 to %d\n", start );
	for ( i = start; i < to_page( VMEM_1_LIMIT, -1 ) - to_page( VMEM_1_BASE, 0 ); i++ ){
		if ( invalidPage1( i ) ){
			struct memory_page * page = get_free_page();
			if ( page == 0 ){
				TracePrintf( 2, "No free pages for stack growth\n" );
				return -1;
			}
			page->protection = PROT_READ | PROT_WRITE;
			modifyPageTable1( i, 1, page->protection, page->frame );
			page->virtual = i;
			table[ i ] = page;
		}
	}
	return 0;
}

/* make some region 1 pages available. see the above note about the TLB
 */
int user_brk( struct memory_page ** pages, int heap_start, int heap_end, void * memory ){
	int memory_page = to_page( (unsigned int) memory, 0 ) - to_page( VMEM_1_BASE, 0 );
	TracePrintf( 8, "User brk. heap start %d. heap end %d. memory %p. memory page %d\n", heap_start, heap_end, memory, memory_page );

	/* allocate some pages of memory */
	if ( memory_page >= heap_end ){
		if ( freePages( memory_page - heap_end + 1) ){
			int i;
			for ( i = heap_end; i <= memory_page; i++ ){
				struct memory_page * page = get_free_page();
				if ( page == 0 ){
					TracePrintf( 4, "No free pages for user brk\n" );
					return ERROR;
				}
				page->virtual = i;
				page->protection = PROT_READ | PROT_WRITE;
				modifyPageTable1( i, 1, page->protection, page->frame );
				pages[ i ] = page;
			}
		} else {
			TracePrintf( 4, "Not enough free memory for user brk\n" );
			return ERROR;
		}
	} else {
		/* deallocate some pages of memory */
		int i;
		for ( i = memory_page + 1; i < heap_end; i++ ){
			add_free_page( pages[ i ] );
			modifyPageTable1( i, 0, pages[ i ]->protection, pages[ i ]->frame );
			pages[ i ] = 0;
		}
	}

	return memory_page + 1;

	/*
	TracePrintf( 3, "User brk to %p\n", memory );
	for ( i = 0; i < to_page( (unsigned int) memory, 0 ) - to_page( VMEM_1_BASE, 0 ); i++ ){
		if ( invalidPage1( i ) ){
			struct memory_page * page = get_free_page();
			if ( page == 0 ){
				TracePrintf( 4, "No free pages for user brk\n" );
				return ERROR;
			}
			page->protection = PROT_READ | PROT_WRITE;
			modifyPageTable1( i, 1, page->protection, page->frame );
		}
	}
	return 0;
	*/
}

/* allocate some memory for the kernel by mapping in pages */
int SetKernelBrk( void * memory ){
	/* the most pages we can allocate is region 0 limit - 4 pages, 2 for stack and 2
	 * for scratch space
	 */
	const int top = VMEM_0_LIMIT - PAGESIZE * 4;
	TracePrintf( 1, "Kernel brk %p\n", memory );
	if ( top_of_kernel_heap >= (void *) top ){
		TracePrintf( 0, "Allocated too much kernel memory %p > %p\n", memory, (void *) top );
		return -1;
	}
	if ( top_of_kernel_heap < memory ){
		top_of_kernel_heap = memory;
	}

	if ( isVirtualMemoryEnabled() ){
		int i;
		for ( i = (int)bottom_of_kernel_heap >> PAGESHIFT; i < (int)top_of_kernel_heap >> PAGESHIFT; i++ ){
			if ( invalidPage0( i ) ){
				struct memory_page * page = get_free_page();
				if ( page == 0 ){
					TracePrintf( 1, "*Warning* Out of memory\n" );
					return -1;
				}
				int num = page->frame;
				page->protection = PROT_READ | PROT_WRITE;
				TracePrintf( 1, "Map in %d to page %p frame %d\n", i, page, num );
				modifyPageTable0( i, 1, page->protection, page->frame );
			}
		}
	}

	return 0;
}

/* map the page to some virtual frame and copy size bytes from block to it.
 * then unmap the page
 */
YalnixError copyBlockToPage( struct memory_page * page, char * block, int size ){
	int virtual = mapUnusedPage0( page->frame, -1 );
	if ( virtual == -1 ){
		TracePrintf( 0, "Could not allocate memory for stack copy\n" );
		return YALNIX_NO_FREE_VIRTUAL_PAGES;
	}
	memcpy( (void *)(virtual << PAGESHIFT), block, size );
	unMapPage0( virtual );
	return YALNIX_NO_ERROR;
}

/* map the kernel stack to pages just below the real kernel stack, 0x7e and 0x7f,
 * then memcpy the new kernel stack to those pages
 *
 * NEVER call this function from anywhere but KernelContextSwitch
 * the kernel stack( pfn 0x7e and 0x7f ) are used by the kernel itself so messing with it
 * via memcpy will corrupt the stack that is in use.
 */
void setupKernelStack( struct memory_page ** stack, int num_pages ){
	unsigned int i;
	for ( i = 0; i < num_pages; i++ ){
		// stack[ i ]->virtual = to_page( KERNEL_STACK_BASE, -(i+1) );	
		stack[ i ]->virtual = to_page( KERNEL_STACK_BASE, i );	
		// printf( "Map user kernel stack to %d %p\n", stack[ i ]->virtual, stack[ i ]->virtual << PAGESHIFT );
		TracePrintf( 6, "Set kernel stack %d physical %d\n", i, stack[ i ]->frame );
		modifyPageTable0( stack[ i ]->virtual, 1, PROT_READ | PROT_WRITE, stack[ i ]->frame );
	}
	flushTLB0();

	/*
	for ( i = 0; i < num_pages; i++ ){
		stack[ i ]->virtual = to_page( KERNEL_STACK_BASE, -(i+1) );	
		modifyPageTable0( stack[ i ]->virtual, 1, PROT_READ | PROT_WRITE, stack[ i ]->frame );
	}
	flushTLB0();

	for ( i = 0; i < num_pages; i++ ){
		copyPage( to_page( KERNEL_STACK_BASE, i ), stack[ i ]->virtual );
	}
	*/
}

/* map valid pages from 'table' in region 1 */
void setupPageTable( struct memory_page ** table, int size ){
	int i;
	for ( i = 0; i < size; i++ ){
		int valid = table[ i ] != 0;
		int frame = table[ i ] != 0 ? table[ i ]->frame : 0;
		int protection = table[ i ] != 0 ? table[ i ]->protection : PROT_READ;
		int virtual = table[ i ] != 0 ? table[ i ]->virtual : i;
		modifyPageTable1( virtual, valid, protection, frame );
	}

	flushTLB1();
}

/* setup virtual memory. map the kernel into region 0 and all unused memory to a
 * free page list. get_free_page will look in that list for free physical pages
 * of memory.
 *
 * allocated_pages uses malloc before virtual memory is set up which is strange
 * so this deserves some explanation. Initially brk() is set to a few bytes above
 * where the _end symbol resides. If _end resides at 0x15000 then brk() might be set
 * with 0x16000 and malloc will start dolling out memory somewhere between _end and
 * this initial brk() value. sbrk(0) will now return 0x16000, clearly above the
 * first memory address that malloc() would have returned. So the data section
 * of the kernel ends at _end but all the memory between _end and sbrk(0) is
 * basically being used by the malloc pool so that memory should be considered
 * used as well. The heap section will start at some bytes above sbrk(0) so
 * it doesn't matter where sbrk(0) actually is which lets me allocate as much
 * memory as I want before finding out where the heap should start. Further calls
 * to malloc will eventually go beyond sbrk(0), the heap start, but at that point
 * virtual memory will be enabled and SetKernelBrk() will be able to map new pages
 * in. All this really means is the data section should have been __data_start to
 * _end but I make it __data_start to sbrk(0) instead so that the memory malloc
 * has already allocated wont be lost.
 */
void initializeVirtualMemory( const unsigned int max_memory ){
	// int counter = 0;
	unsigned int i;

	/*
	extern void * __bss_start;
	extern void * _end;
	*/
	extern void * __data_start;

	/* force kernel brk */
	/*
	void * temp = malloc( 1 );
	temp = temp;
	*/
	/* allocate enough pages to fit all of memory, some of these wont be used */
	struct memory_page * allocated_pages = (struct memory_page *)malloc( sizeof(struct memory_page) * (max_memory >> PAGESHIFT) );
	
	const int data_section = (int)DOWN_TO_PAGE(&__data_start) >> PAGESHIFT;
	int heap_section = (int)UP_TO_PAGE(sbrk(0)) >> PAGESHIFT;
	// const int heap_section = to_page( sbrk( 0 ), 1 );
	// const int heap_section = (int)UP_TO_PAGE(&_end) >> PAGESHIFT;

	// int heap_start = (int)UP_TO_PAGE(sbrk(0)) >> PAGESHIFT;
	const int stack_section_begin = ((int)KERNEL_STACK_BASE >> PAGESHIFT);
	const int stack_section_end = ((int)KERNEL_STACK_LIMIT >> PAGESHIFT);

	bottom_of_kernel_heap = (void *)(heap_section << PAGESHIFT);

	/*
	printf( "data section %d\n", data_section );
	printf( "data end %d\n", DOWN_TO_PAGE(&_end) >> PAGESHIFT );
	printf( "Sbrk(0) = %d\n", heap_section );
	*/

	TracePrintf( 4, "Memory on machine: %u bytes / %u kbytes / %u mbytes\n", max_memory, max_memory / 1024, max_memory / ( 1024 * 1024 ) );

	// printf( "naive malloc = %p\n", malloc( 10 ) );

#if 0
	printf( "Data start %p >> %d\n", &__data_start, data_section );
	printf( "bss %p up to %p page %d\n", &__bss_start, (void *)UP_TO_PAGE( &__bss_start ), (int)&__bss_start >> PAGESHIFT );
	printf( "end %p up to %p page %d\n", &_end, (void *)UP_TO_PAGE( &_end ), (int)&_end >> PAGESHIFT );
	printf( "Sbrk(0) = %p >> %d\n", sbrk(0), heap_section );
	printf( "stack section begin %d << %p\n", stack_section_begin, stack_section_begin << PAGESHIFT );
	printf( "stack section end %d << %p\n", stack_section_end, stack_section_end << PAGESHIFT );
	printf( "bottom of heap %p\n", bottom_of_kernel_heap );
	printf( "Down sbrk %p up sbrk %p sbrk(0) at %d\n", DOWN_TO_PAGE(sbrk(0)), UP_TO_PAGE(sbrk(0)), (int)sbrk(0) >> PAGESHIFT );
	printf( "region0 table %p pfn %d\n", region0_page_table, (int)region0_page_table >> PAGESHIFT );
#endif
	TracePrintf( 3, "Region 0 end %p\n", VMEM_0_LIMIT );

	/* the kernel code */
	for ( i = 0; i < data_section; i++ ){
		// TracePrintf( 3, "Page %d valid addr %p\n", i, i << PAGESHIFT );
		// printf( "Page %d valid addr %p\n", i, i << PAGESHIFT );
		modifyPageTable0( i, 1, PROT_READ | PROT_EXEC, i );
	}

	/* data + bss + the stuff malloc wants to use */
	for ( i = data_section; i < heap_section; i++ ){
		// TracePrintf( 3, "Page %d valid addr %p\n", i, i << PAGESHIFT );
		// printf( "Page %d valid addr %p\n", i, i << PAGESHIFT );
		modifyPageTable0( i, 1, PROT_READ | PROT_WRITE, i );
	}

	/* everything up until heap_section is mapped 1-1 on physical memory */
	for ( i = heap_section; i < max_memory >> PAGESHIFT; i++ ){
		/* dont map the stack space */
		if ( i >= stack_section_begin && i < stack_section_end ){
			continue;
		}
		// struct memory_page * page = (struct memory_page *)malloc( sizeof(struct memory_page) );
		/* I suppose it would be nice to demand that some of these pages be given
		 * to the kernel so that user space doesn't use all of memory..
		 */
		struct memory_page * page = &allocated_pages[ i ];
		*page = (struct memory_page){ .virtual = i, .frame = i, .next = 0 };
		add_free_page( page );
	}

	/* heap pages are invalid */
	for ( i = heap_section; i < stack_section_begin; i++ ){
		modifyPageTable0( i, 0, PROT_READ | PROT_WRITE, 0 );
	}

	/* kernel stack */
	for ( i = stack_section_begin; i < stack_section_end; i++ ){
		TracePrintf( 1, "Map stack %d to %p\n", i, (void *)(i << PAGESHIFT) );
		modifyPageTable0( i, 1, PROT_READ | PROT_WRITE, i );
	}
	
	/* anything left over( in reality nothing ) is invalid as well */
	for ( i = stack_section_end; i < region0_pages; i++ ){
		modifyPageTable0( i, 0, PROT_READ | PROT_WRITE, i );
	}

	WriteRegister( REG_PTBR0, (unsigned int)region0_page_table );
	WriteRegister( REG_PTLR0, region0_pages );

	WriteRegister( REG_PTBR1, (unsigned int)region1_page_table );
	WriteRegister( REG_PTLR1, region1_pages );

	WriteRegister( REG_VM_ENABLE, 1 );
	virtual_memory_enabled = 1;

	/* I doubt this is needed but what the hey */
	WriteRegister( REG_TLB_FLUSH, TLB_FLUSH_ALL );
}

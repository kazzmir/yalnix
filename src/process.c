#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "memory.h"

/* free resources used by a process including
 * - kernel stack
 * - page table array
 * - process struct itself
 */
void freeProcess( struct process * process ){
	int i;
	struct status_list * next = process->terminated.next;
	while ( next != 0 ){
		struct status_list * save = next->next;
		free( next );
		next = save;
	}
	for ( i = 0; i < KERNEL_STACK_PAGES; i++ ){
		if ( process->kernel_stack[ i ] != 0 ){
			add_free_page( process->kernel_stack[ i ] );
		}
	}
	for ( i = 0; i < process->pages; i++ ){
		if ( process->page_table[ i ] != 0 ){
			add_free_page( process->page_table[ i ] );
		}
	}
	free( process->children );
	free( process->page_table );
	free( process );
}

/* insert child into a free space in the parent's children array.
 * the array will grow dynamically if it is too small
 */
int addChildToProcess( struct process * parent, struct process * child ){
	int index = 0;
	/* find a free index in the array */
	for ( index = 0; index < parent->max_children && parent->children[ index ] != 0 ; index++ ){
		/**/
	}

	/* if none were found then grow the array by doubling it */
	if ( index == parent->max_children ){
		struct process ** bigger;
		int old_size = parent->max_children;
		parent->max_children *= 2;
		bigger = (struct process **) malloc( sizeof(struct process *) * parent->max_children );
		if ( ! bigger ){
			parent->max_children = old_size;
			return ERROR;
		}
		bzero( bigger, sizeof(struct process *) * parent->max_children );
		memcpy( bigger, parent->children, sizeof(struct process *) * old_size );
		free( parent->children );
		parent->children = bigger;
	}

	TracePrintf( 3, "Add child %d to parent %d\n", child->id, parent->id );
	parent->children[ index ] = child;

	return 0;
}

/* create an empty process structure
 */
struct process * createProcess(void){
	static unsigned global_process_id = 1;
	int i;
	struct process * process = (struct process *) malloc( sizeof( struct process ) );
	if ( ! process ){
		return 0;
	}

	process->id = global_process_id;
	global_process_id += 1;
	/* -1 is used for ipc so skip it */
	if ( global_process_id == (unsigned)-1 ){
		global_process_id += 1;
	}

	process->status = PROCESS_RUNNABLE;

	process->heap_start = 0;
	process->heap_end = 0;
	process->pages = region1_pages;
	process->parent = 0;
	
	process->terminated = (struct status_list){ .status = 0, .id = 0, .next = 0 };

	process->max_children = 10;
	process->children = (struct process **) malloc( sizeof( struct process * ) * process->max_children );
	if ( ! process->children ){
		free( process );
		return 0;
	}
	bzero( process->children, sizeof( struct process * ) * process->max_children );

	process->page_table = (struct memory_page **) malloc( sizeof( struct memory_page * ) * process->pages );
	if ( ! process->page_table ){
		free( process->children );
		free( process );
		return 0;
	}
	bzero( process->page_table, sizeof(struct memory_page *) * process->pages );

	bzero( process->kernel_stack, sizeof( struct memory_page *) * KERNEL_STACK_PAGES );

	/* kernel stack */
	for ( i = 0; i < KERNEL_STACK_PAGES; i++ ){
		process->kernel_stack[ i ] = get_free_page();
		if ( ! process->kernel_stack[ i ] ){
			freeProcess( process );
			return 0;
		}
		TracePrintf( 5, "pid %d stack %d physical %d\n", process->id, i, process->kernel_stack[ i ]->frame );
	}

	return process;
}

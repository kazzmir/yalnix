/* Yalnix kernel
 * by Jon Rafkind
 */

/* yalnix headers */
#include "yalnix.h"
#include "hardware.h"
#include "memory.h"
#include "debug.h"
#include "process.h"
#include "kernel.h"
#include "schedule.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO:
 * move all the terminal stuff to terminal.c
 * probably move other syscalls to syscalls.c and all the traps to traps.c
 */

struct service{
	/* service port */
	int port;
	/* id of process that registered this service */
	int id;
};

/* array of processes that have regiseterd themselves to provide some service
 * through ipc
 */
static struct service * registered_servers = 0;
static unsigned int max_servers = 0;

/* these bitmasks have to start at 0x10 because they are bitwise or'd
 * with the tty number, i.e. TTY_READ | 1, for reading tty 1
 */
const int TTY_READ = 0x10;
const int TTY_WRITE = 0x20;
const int TTY_WAIT = 0x40;

const int IPC_SEND = 1;
const int IPC_RECEIVE = 2;
const int IPC_REPLY = 3;

/* representation of a terminal in the kernel */
struct tty{
	/* filled when the user types stuff into a terminal */
	char input[ TERMINAL_MAX_LINE ];
	/* filled when a process writes to a terminal */
	char output[ TERMINAL_MAX_LINE ];
	/* number of bytes currently in the *input* buffer
	 * -1 signifies no bytes in the buffer.
	 */
	int bytes;
	/* 1 if this terminal is being used, 0 if free */
	int busy;
};

static struct tty terminals[ NUM_TERMINALS ];

/* a fatal error occured, print a message and halt */
static void panic( YalnixError error ){
	printf( "[kernel] *Fatal error* ");
	printError( error );
	Halt();
}

/* copy kernel stack to some pages */
/*
static void saveStack( struct memory_page ** stack ){
	int i;
	for ( i = 0; i < KERNEL_STACK_PAGES; i++ ){
		copyPage( stack[ i ]->virtual, to_page( KERNEL_STACK_BASE, i ) );
	}
}
*/

/* copy existing kernel stack to some other pages. those pages do not
 * have to be mapped in, copyBlockToPage will map them in, copy data, then unmap them.
 */
static YalnixError copyStack( struct memory_page ** pages, char * stack ){
	int i;
	for ( i = 0; i < KERNEL_STACK_PAGES; i++ ){
		YalnixError error = copyBlockToPage( pages[ i ], stack + PAGESIZE * i, PAGESIZE );
		if ( error != YALNIX_NO_ERROR ){
			return error;
		}
	}
	return YALNIX_NO_ERROR;
}

/* do a kernel context switch
 * 1. save the old kernel context in old process
 * 2. map new kernel stack
 * 3. return new kernel context
 */
static KernelContext * kernelProcess( KernelContext * context, void * p1, void * p2 ){
	struct process * old = (struct process *) p1;
	struct process * new = (struct process *) p2;
	old->kernel_context = *context;

	/* save stack is ok to use because the kernel stack is still mapped in */
	// saveStack( old->kernel_stack );

	setupKernelStack( new->kernel_stack, KERNEL_STACK_PAGES );

	return &new->kernel_context;
}

/* switch to a process without saving the current stack */
static KernelContext * kernelProcessNew( KernelContext * context, void * p1, void * p2 ){
	struct process * new = (struct process *) p2;

	setupKernelStack( new->kernel_stack, KERNEL_STACK_PAGES );
	return &new->kernel_context;
}

/* can switch to a process without copying previous state to the older process
 */
static void switchTo( struct process * old, struct process * new, UserContext * context ){
	TracePrintf( 5, "Switch to old %p new %p\n", old, new );
	if ( old != 0 ){
		old->user_context = *context;
	}

	*context = new->user_context;
	setupPageTable( new->page_table, new->pages );
	if ( old != 0 ){
		KernelContextSwitch( kernelProcess, old, new );
	} else {
		KernelContextSwitch( kernelProcessNew, old, new );
	}

	if ( old != 0 ){
		*context = old->user_context;
		setupPageTable( old->page_table, old->pages );
	}
}

/* switch processes from current process to something else */
static void swapProcesses( UserContext * context ){

	struct process * old, * new;

	TracePrintf( 9, "Get next process\n" );

	getNextProcess( &old, &new );

	/* if there is only one process then don't do anything */
	if ( old != new ){

		old->user_context = *context;
		
		TracePrintf( 2, "Switch from pid %d to pid %d\n", old->id, new->id );
		*context = new->user_context;
		setupPageTable( new->page_table, new->pages );
		KernelContextSwitch( kernelProcess, old, new );
		*context = old->user_context;
		setupPageTable( old->page_table, old->pages );
	}
}

static void swapIfIdle( UserContext * context ){
	if ( isIdle( current_process ) ){
		swapProcesses( context );
	}
}

/* Yalnix traps */
static void dummyTrap( const char * trap, UserContext * context ){
	TracePrintf( 1, "Pid %d Trap '%s'. Program counter( pc ) = %p\n", current_process->process->id, trap, context->pc );
}

/* helper function to call user_brk, which allocates memory in region 1 */
static int allocateUserMemory( struct process * process, unsigned int limit ){
	if ( limit < VMEM_1_LIMIT ){
		int new_heap = user_brk( process->page_table, process->heap_start, process->heap_end, (void *) limit );
		if ( new_heap == ERROR ){
			/* TODO: if brk fails I could try to shrink the pages used
			 * by the stack in an attempt at garbage collecting some
			 * unused space and then calling user_brk again.
			 */
			return ERROR;
		} else {
			process->heap_end = new_heap;
			return 0;
		}
	} else {
		TracePrintf( 11, "[%d] requested sbrk %p above region 1 limit %p\n", process->id, limit, VMEM_1_LIMIT );
		return ERROR;
	}
}

KernelContext * KContextHelper(KernelContext *kContext, void *ptr1, void *ptr2) {
	*((KernelContext*)ptr1) = *kContext;
	memcpy((char *)ptr2,(char *)KERNEL_STACK_BASE, PAGESIZE * 2);
	return (KernelContext*)ptr1;
}

void getCurrentKernelContext( KernelContext * context, char * stack ){
	KernelContextSwitch(KContextHelper, context, stack );
}

/* Copy over all region 1 memory
 */
static YalnixError copyRegion1( struct process * parent, struct process * child ){
	int i;
	YalnixError ret = YALNIX_NO_ERROR;
	child->heap_start = parent->heap_start;
	child->heap_end = parent->heap_end;
	child->stack = parent->stack;
	for ( i = 0; i < parent->pages; i++ ){
		if ( parent->page_table[ i ] != 0 ){
			struct memory_page * page = get_free_page();
			if ( ! page ){
				return YALNIX_OUT_OF_MEMORY;
			}
			child->page_table[ i ] = page;
			TracePrintf( 12, "[%d] Copy region 1 %d from [%d]. Physical %d\n", child->id, i, parent->id, page->frame );
			page->protection = PROT_READ | PROT_WRITE;
			ret = copyBlockToPage( page, (void *)((parent->page_table[ i ]->virtual << PAGESHIFT) + VMEM_1_BASE), PAGESIZE );
			if ( ret != YALNIX_NO_ERROR ){
				TracePrintf( 7, "[%d] Could not copy page\n", child->id );
				return ret;
			}
			page->protection = parent->page_table[ i ]->protection;
			page->virtual = parent->page_table[ i ]->virtual;
		} else {
			child->page_table[ i ] = 0;
		}
	}

	return ret;
}

/* spawn a new process, copy over region 1, and set up the kernel stack
 */
static void handleFork( UserContext * context ){
	struct process * child = createProcess();
	struct process * parent = current_process->process;
	if ( ! child ){
		context->regs[ 0 ] = ERROR;
		return;
	}

	struct process_list * list = (struct process_list *) malloc( sizeof( struct process_list ) );
	if ( ! list ){
		context->regs[ 0 ] = ERROR;
		return;
	}
	list->process = child;
	child->list = list;

	YalnixError ret;
	KernelContext forkContext;
	char * forkStack;
	int parent_id = parent->id;
	int child_id = child->id;
	ret = copyRegion1( parent, child );
	child->user_context = parent->user_context;
	if ( ret != YALNIX_NO_ERROR ){
		TracePrintf( 3, "[%d] fork could not copy %d to %d\n", child_id, parent_id, child_id );
		freeProcess( child );
		free( list );
		context->regs[ 0 ] = ERROR;
		return;
	}
	forkStack = malloc( PAGESIZE * 2 );
	if ( ! forkStack ){
		TracePrintf( 2, "Could not malloc fork stack\n" );
		freeProcess( child );
		free( list );
		context->regs[ 0 ] = ERROR;
		return;
	}

	if ( addChildToProcess( parent, child ) != 0 ){
		TracePrintf( 2, "Could not add child to process\n" );
		freeProcess( child );
		free( forkStack );
		free( list );
		context->regs[ 0 ] = ERROR;
		return;
	}

	addToRunList( list );
	child->parent = parent;

	getCurrentKernelContext( &forkContext, forkStack );
	/* current_process is different depending on if the parent or child
	 * is executing
	 */
	TracePrintf( 2, "[%d] In fork\n", current_process->process->id );

	/* only copy the kernel stack to the new process if we are the parent
	 * that is still executing. the child cannot execute until this occurs anyway
	 */
	if ( current_process->process->id == parent_id ){
		child->kernel_context = forkContext;
		ret = copyStack( child->kernel_stack, forkStack );
		free( forkStack );
		/* return for parent is the pid of the child */
		context->regs[ 0 ] = child_id;
	} else {
		/* return for child is 0 */
		context->regs[ 0 ] = 0;
	}
}

/* take parent out of whatever list its currently in and put
 * it back into the run list, by appending it to current_process
 */
static void wakeupParent( struct process * parent ){
	struct process_list * list = parent->list;
	if ( list->next ){
		list->next->prev = list->prev;
	}
	if ( list->prev ){
		list->prev->next = list->next;
	}
	list->next = current_process->next;
	list->prev = current_process;
	current_process->next->prev = list;
	current_process->next = list;
}

static void notifyParentOfDeath( struct process * parent, struct process * child, int code ){
	int index = 0;
	struct status_list * status;

	for ( index = 0; index < parent->max_children && parent->children[ index ] != child; index++ ){
	}

	if ( index == parent->max_children ){
		/* this should never happen */
		panic( YALNIX_INVALID_CHILD );
	}

	/* put the parent back on the run list */
	wakeupParent( parent );

	/* child is not alive anymore, so mark its index as dead */
	parent->children[ index ] = 0;

	/* add an object to the list of children that have terminated */
	status = (struct status_list *) malloc( sizeof(struct status_list) );
	if ( status ){
		status->status = code;
		status->id = child->id;
		status->next = parent->terminated.next;
		parent->terminated.next = status;
	}
	/* if status was 0, meaning malloc failed, then in the worst case
	 * the parent will get an error when he calls Wait(). oh well!
	 */
}

/* relinquish control from all ports owned by this process */
static void deregisterServers( struct process * process ){
	int id = process->id;
	int i = 0;
	for ( i = 0; i < max_servers; i++ ){
		if ( registered_servers[ i ].id == id ){
			registered_servers[ i ] = (struct service) { .port = 0, .id = -1 };
		}
	}
}

/* a process died for whatever reason. notify the parent of its
 * untimely death and clean up and resources it used
 */
static void doExit( struct process * process, int code ){
	int i;
	TracePrintf( 6, "[%d] exiting with code %d\n", process->id, code );
	if ( process->parent ){
		notifyParentOfDeath( process->parent, process, code );
	}
	process->status = PROCESS_DIED;

	for ( i = 0; i < process->max_children; i++ ){
		/* make children orphans. sorry kids! */
		if ( process->children[ i ] ){
			process->children[ i ]->parent = 0;
		}
	}

	deregisterServers( process );

	removeProcess( process );
}

/* overlay a new program onto the current process
 */
static int handleExec( UserContext * context ){
	char * filename = (char *)context->regs[ 0 ];
	char ** args = (char **) context->regs[ 1 ];
	char ** f;
	int i;
	struct process * process = current_process->process;
	int error = 0;
	/* if the user passed in arguments not in region 1 then fail */
	if ( ! ensureRegion1StringRead( filename ) ){
		return ERROR;
	}
	for ( f = args; *f != 0; f++ ){
		if ( ! ensureRegion1Read( f, sizeof(char**) ) ){
			return ERROR;
		}
		if ( ! ensureRegion1StringRead( *f ) ){
			return ERROR;
		}
	}

	char * keep = filename;
	/* all arguments from the user have to be copied into some scratch space */
	filename = (char *) malloc( strlen( keep ) + 1 );
	if ( ! filename ){
		return ERROR;
	}
	strcpy( filename, (char *) context->regs[ 0 ] );

	i = 0;
	for ( ; *args != 0; args++, i++ ){
		/**/
	}
	
	args = (char **) malloc( sizeof(char *) * i + 1 );
	if ( ! args ){
		free( filename );
		return ERROR;
	}
	/* copy argv */
	f = (char **) context->regs[ 1 ];
	for ( i = 0; f[ i ] != 0; i++ ){
		args[ i ] = malloc( strlen( f[ i ] ) + 1 );
		if ( ! args[ i ] ){
			int q = 0;
			for ( q = 0; q < i; q++ ){
				free( args[ q ] );
				free( args );
				free( filename );
				return ERROR;
			}
		}
		strcpy( args[ i ], f[ i ] );
	}
	args[ i ] = 0;

	TracePrintf( 2, "[%d] Try to exec %s\n", process->id, filename );

	error = loadProgram( process, filename, args );
	TracePrintf( 2, "[%d] Exec result %d\n", process->id, error );
	if ( error == YALNIX_INVALID_PROGRAM ){
		TracePrintf( 2, "[%d] Could not exec '%s'\n", process->id, filename );
		free( filename );
		for ( i = 0; args[ i ] != 0; i++ ){
			free( args[ i ] );
		}
		free( args );
		struct process_list * save = current_process->next;
		doExit( process, ERROR );
		current_process = save;
		// swapProcesses( context );
		switchTo( 0, current_process->process, context );
		/* doesn't get here */
	} else if ( error == 0 ){
		*context = process->user_context;
	}
	
	for ( i = 0; args[ i ] != 0; i++ ){
		free( args[ i ] );
	}
	
	free( args );
	free( filename );

	return error;
}

static void handleExit( struct process * process, int code ){
	doExit( process, code );
}

/* returns the number of non-0 pointers in obj */
static int countObjects( void ** obj, int max ){
	int count = 0;
	int i;
	for ( i = 0; i < max; i++ ){
		if ( obj[ i ] ){
			count += 1;
		}
	}
	return count;
}

/* wait for dead children */
static int handleWait( UserContext * context, struct process * process, int * status ){
	if ( ! ensureRegion1ReadWrite( status, sizeof( int ) ) ){
		return ERROR;
	}

	/* a goto here is more convenient than a while loop. if the process
	 * goes to sleep because its waiting for a child to die then it will
	 * jump back to here after being woken up. besides goto's make me
	 * feel like I'm using tail recursion.
	 */
	try_again:
	TracePrintf( 4, "[%d] waiting for dead children\n", process->id );
	/* see if any children have died so we can return one of them */
	if ( process->terminated.next != 0 ){
		struct status_list * list = process->terminated.next;
		int id = list->id;
		process->terminated.next = list->next;
		*status = list->status;
		free( list );
		return id;
	}

	/* otherwise wait for a child to die */
	if ( countObjects( (void **) process->children, process->max_children ) > 0 ){
		/* dont run until a child has died */

		struct process_list * save = current_process->next;
		struct process * old = current_process->process;
		struct process_list * list = process->list;
		if ( list->prev ){
			list->prev->next = list->next;
		}
		if ( list->next ){
			list->next->prev = list->prev;
		}

		addToBusyList( list );

		current_process = skipIdle( save );
		switchTo( old, current_process->process, context );
		/* when the process comes back it will reap some children */

		goto try_again;
	}

	TracePrintf( 2, "[%d] No children left\n", process->id );

	return ERROR;
}

/* put process in the delay list and take it off of the run queue */
static int handleDelay( struct process * process, int time ){
	struct process_list * list = process->list;

	if ( time == 0 ){
		return 0;
	}

	if ( time < 0 ){
		return ERROR;
	}

	int * delay = (int *) malloc( sizeof( int ) );

	if ( ! delay ){
		return ERROR;
	}

	if ( list->prev ){
		list->prev->next = list->next;
	}
	if ( list->next ){
		list->next->prev = list->prev;
	}

	*delay = time;
	list->obj = delay;

	addToDelayedList( list );

	TracePrintf( 4, "[%d] Delay for %d ticks\n", process->id, time );

	return 1;
}

/* Copy max of (terminal->bytes, length) bytes into buffer.
 * If length is less than terminal->bytes than the bytes from
 * length to terminal->bytes is shifted left length bytes.
 */
static int copyFromTty( struct tty * terminal, void * buffer, int length ){
	int read = 0;
	if ( length == terminal->bytes ){
		memcpy( buffer, terminal->input, length );
		read = terminal->bytes;
		terminal->bytes = -1;
	} else if ( length < terminal->bytes ){
		char tmp[ TERMINAL_MAX_LINE ];
		memcpy( buffer, terminal->input, length );
		read = length;
		memcpy( tmp, terminal->input + length, terminal->bytes - length );
		memcpy( terminal->input, tmp, terminal->bytes - length );
		terminal->bytes -= read;
	} else if ( length > terminal->bytes ){
		memcpy( buffer, terminal->input, terminal->bytes );
		read = terminal->bytes;
		terminal->bytes = -1;
	}
	return read;
}

/* read from bytes from a terminal. this process will sleep if
 * the terminal is in use.
 */
static int handleTtyRead( UserContext * context, struct process * process, int tty, void * buffer, int length ){
	if ( ! ensureRegion1ReadWrite( buffer, length ) ){
		return ERROR;	
	}

	if ( tty >= 0 && tty < NUM_TERMINALS ){
		struct tty * terminal = &terminals[ tty ];
		try_again:
		/* something is already in the terminal */
		if ( terminal->bytes != -1 ){
			return copyFromTty( terminal, buffer, length );
		} else {
			/* otherwise add this process to the i/o list */
			int * tty_num = (int *) malloc( sizeof(int) );
			if ( ! tty_num ){
				return -1;
			}

			*tty_num = tty | TTY_READ;

			struct process_list * save = current_process->next;
			struct process * old = current_process->process;

			struct process_list * list = process->list;
			if ( list->prev ){
				list->prev->next = list->next;
			}
			if ( list->next ){
				list->next->prev = list->prev;
			}

			/* keep track of the tty for this process */
			list->obj = tty_num;

			addToIoList( list );

			current_process = skipIdle( save );
			switchTo( old, current_process->process, context );
			/* when this process comes back it will try to read from the tty again */
			goto try_again;
		}
	} else {
		return ERROR;
	}
}

/* a process wants to write into a tty. if the tty is free then send the write along
 * via TtyTransmit and add the process to the io list waiting for the transmit to finish.
 * if the tty is busy then add the process to the io list waiting for the tty to become free
 */
static int handleTtyWrite( UserContext * context, struct process * process, int tty, void * buffer, int length ){
	if ( ! ensureRegion1Read( buffer, length ) ){
		return ERROR;
	}
	if ( length == 0 ){
		return ERROR;
	}
	if ( tty >= 0 && tty < NUM_TERMINALS ){
		struct tty * terminal = &terminals[ tty ];
		int * tty_num = (int *) malloc( sizeof(int) );
		if ( ! tty_num ){
			return ERROR;
		}

		/* wait for terminal to become free. when the process wakes up
		 * the terminal is not necessarily free, so it has to be continually
		 * checked.
		 */
		while ( terminal->busy ){
			TracePrintf( 4, "[%d] waiting for tty %d\n", process->id, tty );
			/*
			swapProcesses( context );
			*/
			struct process_list * save = current_process->next;
			struct process * old = current_process->process;

			struct process_list * list = process->list;
			if ( list->prev ){
				list->prev->next = list->next;
			}
			if ( list->next ){
				list->next->prev = list->prev;
			}

			*tty_num = tty | TTY_WAIT | TTY_WRITE;

			/* keep track of the tty for this process */
			list->obj = tty_num;

			addToIoList( list );

			current_process = skipIdle( save );
			switchTo( old, current_process->process, context );
		}

		TracePrintf( 3, "[%d] writing to terminal %d\n", process->id, tty );

		terminal->busy = 1;
		if ( length > TERMINAL_MAX_LINE ){
			length = TERMINAL_MAX_LINE;
		}
		memcpy( terminal->output, buffer, length );
		TtyTransmit( tty, terminal->output, length );

		/* move process of run list into io list */
		struct process_list * save = current_process->next;
		struct process * old = current_process->process;

		struct process_list * list = process->list;
		if ( list->prev ){
			list->prev->next = list->next;
		}
		if ( list->next ){
			list->next->prev = list->prev;
		}
		
		*tty_num = tty | TTY_WRITE;

		/* keep track of the tty for this process */
		list->obj = tty_num;

		addToIoList( list );

		current_process = skipIdle( save );
		switchTo( old, current_process->process, context );
		/* once we got here the tty interrupt fired */
		
		TracePrintf( 3, "[%d] wrote to terminal %d\n", process->id, tty );

		terminal->busy = 0;
	} else {
		return ERROR;
	}

	return length;
}

/* remove 'list' from the linked list it currently sits in and place it
 * in the list pointed to by head.
 * returns the node that immediately followed 'list' before removal
 * This is useful for taking the current process off the run queue,
 * putting it on some other queue and returning the next node on the run queue.
 */
 /*
static struct process_list * putProcessOnList( struct process_list * head, struct process_list * list ){
	struct process_list * save = list->next;
	if ( list->prev ){
		list->prev->next = list->next;
	}
	if ( list->next ){
		list->next->prev = list->prev;
	}

	list->next = head->next;
	list->prev = head;
	if ( list->next ){
		list->next->prev = list;
	}
	head->next = list;
	return save;
}
*/

/* find the process receiving with id 'id' */
static struct process_list * findReceiver( int id ){
	return findIpc( -1, -1, id );
}

/* find a process with id 'to' that only wants to receive from 'from' */
static struct process_list * findReceiverSpecific( int from, int to ){
	return findIpc( -1, from, to );
}

/* find a process that wants to send to id 'id' */
static struct process_list * findSender( int id, int from ){
	return findIpc( id, from, -1 );
}

/* return the service, id/port pair, given the port */
static struct service * findService( int port ){
	int i = 0;
	for ( i = 0; i < max_servers; i++ ){
		if ( registered_servers[ i ].id != -1 &&
		     registered_servers[ i ].port == port ){
			return &registered_servers[ i ];
		}
	}

	return 0;
}

/* return the pid for the service that registered the port
 * or -1 if there is no available service
 */
static int lookupService( int port ){
	struct service * s = findService( port );
	if ( s != 0 ){
		return s->id;
	} else {
		return -1;
	}
}

/* Send an ipc message to some other process.
 * This process tries to find a process on the ipc list waiting to
 * sent to. If it finds one then this process copies its message into
 * the receiving processes's inbox and then goes to sleep waiting for
 * a reply. If it does not find a process it goes to sleep waiting
 * for a process that does want to receive a message with a matching
 * id to wake this process up.
 */
static int handleSend( UserContext * context, caddr_t message, int to ){
	TracePrintf( 4, "[%d] Send a message %p to %d\n", current_process->process->id, message, to );

	if ( ! ensureRegion1ReadWrite( message, IPC_MAX_LENGTH ) ){
		return ERROR;
	}

	if ( to < 0 ){
		int port = -to;
		to = lookupService( port );
		if ( to == -1 ){
			TracePrintf( 4, "[%d] No server registered on port %d\n", current_process->process->id, port );
			return ERROR;
		}
	}

	struct ipc * obj = (struct ipc *) malloc( sizeof(struct ipc) );
	if ( ! obj ){
		return ERROR;
	}

	int sent = 0;
	while ( ! sent ){
		TracePrintf( 5, "[%d] trying to send ipc message to %d\n", current_process->process->id, to );
		struct process_list * receiver = findReceiver( to );
		if ( receiver ){
			memcpy( receiver->process->inbox, message, IPC_MAX_LENGTH );
			sent = 1;

			struct ipc * obj = (struct ipc *) receiver->obj;
			obj->from = current_process->process->id;
			/* wake the receiver up */
			// free( receiver->obj );
			if ( receiver->prev ){
				receiver->prev->next = receiver->next;
			}
			if ( receiver->next ){
				receiver->next->prev = receiver->prev;
			}
			receiver->next = current_process->next;
			receiver->prev = current_process;
			current_process->next->prev = receiver;
			current_process->next = receiver;
		} else {
			/* goto sleep */
			obj->from = current_process->process->id;
			obj->to = to;
			obj->receive = -1;
			current_process->obj = obj;
			struct process_list * save = current_process->next;
			struct process * old = current_process->process;
			if ( current_process->prev ){
				current_process->prev->next = current_process->next;
			}
			if ( current_process->next ){
				current_process->next->prev = current_process->prev;
			}

			addToIpcList( current_process );

			current_process = skipIdle( save );
			switchTo( old, current_process->process, context );
		}
	}

	/* the message has been sent so now go back to sleep until a reply
	 * has been sent
	 */

	obj->receive = current_process->process->id;
	obj->to = -1;
	/* not explicitly mentioned in the docs but this process should only
	 * get the reply from the process it just sent to.
	 */
	obj->from = to;
	current_process->obj = obj;

	struct process_list * save = current_process->next;
	struct process * old = current_process->process;
	if ( current_process->prev ){
		current_process->prev->next = current_process->next;
	}
	if ( current_process->next ){
		current_process->next->prev = current_process->prev;
	}
	addToIpcList( current_process );
	// current_process = skipIdle( putProcessOnList( &ipc_list, current_process ) );
	current_process = skipIdle( save );

	switchTo( old, current_process->process, context );
	memcpy( message, current_process->process->inbox, IPC_MAX_LENGTH );
	free( obj );
	current_process->obj = 0;

	return 0;
}

static void wakeupSenders( int to, int from ){
	struct process_list * sender = findSender( to, from );
	if ( sender ){
		if ( sender->prev ){
			sender->prev->next = sender->next;
		}
		if ( sender->next ){
			sender->next->prev = sender->prev;
		}
		sender->next = current_process->next;
		sender->prev = current_process;
		current_process->next->prev = sender;
		current_process->next = sender;
		TracePrintf( 5, "[%d] Woke up %p %d\n", current_process->process->id, sender->process, sender->process->id );
	}
}

/* receive an ipc message.
 * First wake up a process that might be trying to send to this process.
 * Then this process adds itself to the ipc list waiting for the sender to
 * push his message out. The sender will wake this process back up and
 * finally the sender's pid is returned.
 *
 * If from is -1 then any process can send to this one. If it is some positive
 * number then only the process with id 'from' can send to it.
 */
static int handleReceive( UserContext * context, char * buffer, int from ){
	
	if ( ! ensureRegion1ReadWrite( buffer, IPC_MAX_LENGTH ) ){
		return ERROR;
	}

	struct ipc * obj = (struct ipc *) malloc( sizeof( struct ipc ) );
	if ( ! obj ){
		return ERROR;
	}

	obj->receive = current_process->process->id;
	obj->to = -1;
	obj->from = from;

	current_process->obj = obj;

	/* wake up any processes that want to send to this process */
	wakeupSenders( current_process->process->id, from );

	/* put self in the ipc list and goto sleep. the sender will
	 * wake this process back up.
	 */
	struct process_list * save = current_process->next;
	struct process * old = current_process->process;
	if ( current_process->prev ){
		current_process->prev->next = current_process->next;
	}
	if ( current_process->next ){
		current_process->next->prev = current_process->prev;
	}

	addToIpcList( current_process );

	current_process = skipIdle( save );
	TracePrintf( 5, "[%d] waiting to receive\n", old->id );
	switchTo( old, current_process->process, context );

	memcpy( buffer, current_process->process->inbox, IPC_MAX_LENGTH );

	obj = (struct ipc *) current_process->obj;
	int id = obj->from;
	free( obj );
	current_process->obj = 0;

	return id;
}

/* send an ipc message back to some process.
 * if there is no process waiting for a reply then return ERROR.
 * otherwise copy the message to that process's inbox and put
 * the process back on the run queue.
 */
static int handleReply( UserContext * context ){
	char * message = (char *) context->regs[ 0 ];	
	int to = context->regs[ 1 ];

	if ( ! ensureRegion1Read( message, IPC_MAX_LENGTH ) ){
		return ERROR;
	}

	struct process_list * list = findReceiverSpecific( current_process->process->id, to );
	if ( ! list ){
		/* no one is waiting for a reply */
		return -1;
	} else {
		if ( list->prev ){
			list->prev->next = list->next;
		}
		if ( list->next ){
			list->next->prev = list->prev;
		}
		list->next = current_process->next;
		list->prev = current_process;
		current_process->next->prev = list;
		current_process->next = list;

		memcpy( list->process->inbox, message, IPC_MAX_LENGTH );
	}

	return 0;
}

/* register a service on a given port */
static int handleRegister( unsigned int port ){
	if ( findService( port ) ){
		return ERROR;
	}

	if ( ! registered_servers ){
		max_servers = 4;
		registered_servers = (struct service *) malloc( sizeof( struct service ) * max_servers );
		if ( ! registered_servers ){
			return ERROR;
		}
		int i;
		for ( i = 0; i < max_servers; i++ ){
			registered_servers[i] = (struct service){ .port = 0, .id = -1 };
		}
	}

	int index = 0;
	for ( index = 0; index < max_servers && registered_servers[ index ].id != -1; index++ ){
	}

	/* grow the servers list */
	if ( index == max_servers ){
		int new_max = max_servers * 2;
		struct service * new_servers = (struct service *) malloc( sizeof( struct service ) * new_max );
		if ( ! new_servers ){
			return ERROR;
		}
		int i;
		for ( i = max_servers; i < new_max; i++ ){
			registered_servers[i] = (struct service){ .port = 0, .id = -1 };
		}
		memcpy( new_servers, registered_servers, sizeof( struct service ) * max_servers );
		free( registered_servers );
		registered_servers = new_servers;
		max_servers = new_max;
	}

	registered_servers[ index ] = (struct service){ .port = port, .id = current_process->process->id };
	return 0;
}

/* copy data in process space from sourceid to destid.
 * This function doesn't care if either process is currently mapped in,
 * it will map each frame that needs to be copied to some unused virtual
 * page in region 0 and copy the bytes from there.
 *
 * All the src_/dest_ variables are for when the addresses don't align
 * within their respective pages. e.g:
 * dest = 0x5140
 * src = 0xe523
 * And assume there is a page boundary at 0x5000, 0x6000, 0xe000, 0xf000.
 * Then there are less bytes from 0xf000 - src than there are in
 * 0x6000 - dest.
 */
static int copyProcessSpace( int destid, int sourceid, caddr_t dest, caddr_t src, int length ){
	struct process * source_process = findProcess( sourceid );
	struct process * dest_process = findProcess( destid );

	TracePrintf( 6, "[%d] Copy user space: dest id %d dest addr %p source id %d source addr %p length %d\n", current_process->process->id, destid, dest, sourceid, src, length );

	if ( ! source_process || ! dest_process ){
		return ERROR;
	}

	if ( ! ensureReadInTable( source_process->page_table, src, length ) ){
		TracePrintf( 6, "[kernel] %p is not a valid address in the source process\n", src );
		return ERROR;
	}

	if ( ! ensureReadWriteInTable( dest_process->page_table, dest, length ) ){
		TracePrintf( 6, "[kernel] %p is not a valid address in the destination process\n", dest );
		return ERROR;
	}

	/* find an unused page in region 0 to do the copying  */
	int dest_virtual = findUnusedPage0();
	if ( dest_virtual == -1 ){
		return ERROR;
	}
	TracePrintf( 8, "Dest virtual %d\n", dest_virtual );
	mapUnusedPage0( 0, dest_virtual );
	int src_virtual = findUnusedPage0();
	if ( src_virtual == -1 ){
		unMapPage0( dest_virtual );
		return ERROR;
	}
	TracePrintf( 8, "Src virtual %d\n", src_virtual );
	mapUnusedPage0( 0, src_virtual );
	
	/* current page in process space for each process */
	int dest_page = to_page( (unsigned long) dest, 0 ) - to_page( VMEM_1_BASE, 0 );
	int src_page = to_page( (unsigned long) src, 0 ) - to_page( VMEM_1_BASE, 0 );
		
	/* max bytes that can be copied from this page */
	int dest_max = length > (unsigned long)UP_TO_PAGE( dest + 1 ) - (long)dest ? (unsigned long)UP_TO_PAGE( dest + 1 ) - (long)dest : length;
	/* starting address within the page */
	int dest_begin = (unsigned long) dest - (unsigned long) DOWN_TO_PAGE( dest );

	int source_max = length > (unsigned long) UP_TO_PAGE( src + 1 ) - (unsigned long) src ? (unsigned long) UP_TO_PAGE( src + 1 ) - (unsigned long) src : length;
	int source_begin = (unsigned long) src - (unsigned long) DOWN_TO_PAGE( src );
		
	/* map the same physical frame being used by the process to the region 0 page */		
	mapUnusedPage0( dest_process->page_table[ dest_page ]->frame, dest_virtual );
	mapUnusedPage0( source_process->page_table[ src_page ]->frame, src_virtual );

	TracePrintf( 7, "Map dest virtual %d to physical %d\n", dest_virtual, dest_page );
	TracePrintf( 7, "Map src virtual %d to physical %d\n", src_virtual, src_page );

	while ( length > 0 ){
		/* copy the fewest allowed bytes */
		int min = dest_max < source_max ? dest_max : source_max;

		unsigned long dest_addr = (dest_virtual << PAGESHIFT) + dest_begin;
		unsigned long src_addr = (src_virtual << PAGESHIFT) + source_begin;

		length -= min;
		TracePrintf( 7, "Copy %d bytes from %p to %p. Remaining %d\n", min, src_addr, dest_addr, length );

		memcpy( (void *) dest_addr, (void *) src_addr, min );

		if ( length == 0 ){
			break;
		}

		dest_max -= min;
		source_max -= min;
		dest_begin += min;
		source_begin += min;
		TracePrintf( 7, "Dest page %d max %d dest begin %d\n", dest_page, dest_max, dest_begin );
		TracePrintf( 7, "Src page %d max %d src begin %d\n", src_page, source_max, source_begin );
		/* if dest ran out of bytes to copy then find the next page */
		if ( dest_max == 0 ){
			dest_page += 1;
			dest_begin = 0;
			dest_max = length < PAGESIZE ? length : PAGESIZE;
			mapUnusedPage0( dest_process->page_table[ dest_page ]->frame, dest_virtual );
		}

		/* same for src */
		if ( source_max == 0 ){
			src_page += 1;
			source_begin = 0;
			source_max = length < PAGESIZE ? length : PAGESIZE;
			mapUnusedPage0( source_process->page_table[ src_page ]->frame, src_virtual );
		}
	}

	/* dont leave bogus pages mapped into region 0 */
	unMapPage0( dest_virtual );
	unMapPage0( src_virtual );

	return 0;
}

static int handleCopyFrom( int srcpid, caddr_t dest, caddr_t src, int length ){
	if ( findIpc( -1, current_process->process->id, srcpid ) == 0 ){
		return ERROR;
	}
	return copyProcessSpace( current_process->process->id, srcpid, dest, src, length );
}

static int handleCopyTo( int destid, caddr_t dest, caddr_t src, int length ){
	if ( findIpc( -1, current_process->process->id, destid ) == 0 ){
		return ERROR;
	}
	return copyProcessSpace( destid, current_process->process->id, dest, src, length );
}

static int handleReadSector( UserContext * context, int sector, caddr_t dest ){
	if ( ! ensureRegion1Read( dest, SECTORSIZE ) ){
		return ERROR;
	}

	if ( sector < 1 || sector >= NUMSECTORS ){
		return ERROR;
	}

	char * buffer = (char *) malloc( SECTORSIZE );
	if ( ! buffer ){
		return ERROR;
	}
	bzero( buffer, SECTORSIZE );

	DiskAccess( DISK_READ, sector, buffer );

	struct process_list * save = current_process->next;
	struct process * old = current_process->process;
	if ( current_process->prev ){
		current_process->prev->next = current_process->next;
	}
	if ( current_process->next ){
		current_process->next->prev = current_process->prev;
	}

	addToDiskList( current_process );

	current_process = skipIdle( save );
	TracePrintf( 5, "[%d] waiting for disk read\n", old->id );
	switchTo( old, current_process->process, context );

	memcpy( dest, buffer, SECTORSIZE );
	free( buffer );

	return 0;
}

static int handleWriteSector( UserContext * context, int sector, caddr_t src ){
	if ( ! ensureRegion1ReadWrite( src, SECTORSIZE ) ){
		return ERROR;
	}
	
	if ( sector < 1 || sector >= NUMSECTORS ){
		return ERROR;
	}

	char * buffer = (char *) malloc( SECTORSIZE );
	if ( ! buffer ){
		return ERROR;
	}
	memcpy( buffer, src, SECTORSIZE );

	DiskAccess( DISK_WRITE, sector, buffer );

	struct process_list * save = current_process->next;
	struct process * old = current_process->process;
	if ( current_process->prev ){
		current_process->prev->next = current_process->next;
	}
	if ( current_process->next ){
		current_process->next->prev = current_process->prev;
	}

	addToDiskList( current_process );

	current_process = skipIdle( save );
	TracePrintf( 5, "[%d] waiting for disk write\n", old->id );
	switchTo( old, current_process->process, context );

	free( buffer );

	return 0;
}

static void kernelTrap( UserContext * context ){
	TracePrintf( 8, "[%d] Kernel trap syscall vector %d code %p pc %p\n", current_process->process->id, context->vector, (void *) context->code, context->pc );
	switch ( context->code ){
		case YALNIX_FORK : {
			handleFork( context );
			break;
		}
		case YALNIX_EXEC : {
			context->regs[ 0 ] = handleExec( context );
			break;
		}
		case YALNIX_DELAY : {
			struct process_list * save = current_process->next;
			struct process * old = current_process->process;

			int ret = handleDelay( current_process->process, context->regs[ 0 ] );
			context->regs[ 0 ] = ret;
			if ( ret > 0 ){
				context->regs[ 0 ] = 0;
				current_process = skipIdle( save );
				// TracePrintf( 3, "Delay switch to %d\n", current_process->process->id );
				switchTo( old, current_process->process, context );
			}
			break;
		}
		case YALNIX_TTY_READ : {
			int tty = context->regs[ 0 ];
			void * buf = (void *)context->regs[ 1 ];
			int length = context->regs[ 2 ];

			context->regs[ 0 ] = handleTtyRead( context, current_process->process, tty, buf, length );

			break;
		}
		case YALNIX_TTY_WRITE : {
			int tty = context->regs[ 0 ];
			void * buf = (void *)context->regs[ 1 ];
			int length = context->regs[ 2 ];

			context->regs[ 0 ] = handleTtyWrite( context, current_process->process, tty, buf, length );
			break;
		}
		case YALNIX_GETPID : {
			context->regs[ 0 ] = current_process->process->id;
			break;
		}
		case YALNIX_BRK : {
			int ret = allocateUserMemory( current_process->process, context->regs[ 0 ] );
			context->regs[ 0 ] = ret;
			break;
		}
		case YALNIX_READ_SECTOR : {
			context->regs[ 0 ] = handleReadSector( context, context->regs[ 0 ], (caddr_t) context->regs[ 1 ] );
			break;
		}
		case YALNIX_WRITE_SECTOR : {
			context->regs[ 0 ] = handleWriteSector( context, context->regs[ 0 ], (caddr_t) context->regs[ 1 ] );
			break;
		}
		case YALNIX_SEND : {
			context->regs[ 0 ] = handleSend( context, (caddr_t) context->regs[ 0 ], (int) context->regs[ 1 ] );
			break;
		}
		case YALNIX_RECEIVE : {
			context->regs[ 0 ] = handleReceive( context, (char *) context->regs[ 0 ], -1 );
			break;
		}
		case YALNIX_RECEIVESPECIFIC : {
			context->regs[ 0 ] = handleReceive( context, (char *) context->regs[ 0 ], (int) context->regs[ 1 ] );
			break;
		}
		case YALNIX_REPLY : {
			context->regs[ 0 ] = handleReply( context );
			break;
		}
		case YALNIX_REGISTER : {
			context->regs[ 0 ] = handleRegister( context->regs[ 0 ] );
			break;
		}
		case YALNIX_COPY_FROM : {
			context->regs[ 0 ] = handleCopyFrom( context->regs[ 0 ], (caddr_t) context->regs[ 1 ], (caddr_t) context->regs[ 2 ], context->regs[ 3 ] );
			break;
		}
		case YALNIX_COPY_TO : {
			context->regs[ 0 ] = handleCopyTo( context->regs[ 0 ], (caddr_t) context->regs[ 1 ], (caddr_t) context->regs[ 2 ], context->regs[ 3 ] );
			break;
		}
		case YALNIX_WAIT : {
			int * status = (int *) context->regs[ 0 ];
			context->regs[ 0 ] = handleWait( context, current_process->process, status );
			break;
		}
		case YALNIX_EXIT : {
			struct process_list * save = current_process->next;
			handleExit( current_process->process, context->regs[ 0 ] );
			current_process = skipIdle( save );
			/*
			TracePrintf( 3, "Process %d is exiting\n", current_process->process->id );
			current_process->process->status = PROCESS_DIED;
			*/
			// swapProcesses( context );
			switchTo( 0, current_process->process, context );
			break;
		}
		default : {
			printf( "Warning: unimplemented syscall %p\n", (void *)(context->code & YALNIX_MASK) );
			context->regs[ 0 ] = ERROR;
			break;
		}
	}
}

/* update the counter that delayed processes have.
 * when the counter hits 0 make the process runnable again.
 */
static void updateDelayedProcesses(){
	// struct process_list * list = delayed_list.next;
	struct process_list * list = getDelayedList();

	/* count down delay time for processes that called the delay system call */
	while ( list != 0 ){
		struct process_list * save = list->next;
		int * delay = (int *) list->obj;
		*delay -= 1;
		if ( *delay <= 0 ){
			TracePrintf( 6, "[%d] take off delay list\n", list->process->id );
			free( delay );
			list->obj = 0;
			if ( list->next ){
				list->next->prev = list->prev;
			}
			if ( list->prev ){
				list->prev->next = list->next;
			}

			/*
			list->next = current_process->next;
			list->prev = current_process;
			current_process->next->prev = list;
			current_process->next = list;
			*/
			addToRunList( list );
		}

		list = save;
	}
}

/* update delayed processes and swap to the next process */
static void clockTrap( UserContext * context ){
	TracePrintf( 6, "Clock trap\n" );
	updateDelayedProcesses();
	swapProcesses( context );
}

/* process did something illegal, kill it */
static void illegalTrap( UserContext * context ){
	// dummyTrap( "illegal", context );
	printf( "Process pid %d performed an illegal operation\n", current_process->process->id );
	struct process_list * save = current_process->next;
	doExit( current_process->process, ERROR );
	current_process = skipIdle( save );
	switchTo( 0, current_process->process, context );
}

/* return 1 if the address is probably stack growth
 * sp - 4 is due to the return address that the callee will
 * place on the stack for function invocation before jumping
 * to the function.
 */
static int isStackGrowth( UserContext * context ){
	return context->addr >= (void *)(context->sp - 4) &&
	       (void *)DOWN_TO_PAGE( context->addr ) > (void *)VMEM_1_BASE &&
	       context->addr < (void *)VMEM_1_LIMIT;
}

/* grow the stack or kill the process due to illegal memory address */
static void memoryTrap( UserContext * context ){
	TracePrintf( 6, "[%d] memory trap region 1 %p to %p. addr %p page %d stack %d base %p pc %p heap_end %p\n", current_process->process->id, VMEM_1_BASE, VMEM_1_LIMIT, context->addr, to_page( (unsigned int) context->addr, 0 ), current_process->process->stack, context->ebp, context->pc, (void *)((current_process->process->heap_end << PAGESHIFT) + VMEM_1_BASE) );

	/* if its just stack growth because the addr is close enough to sp then
	 * grant more memory
	 */
	if ( isStackGrowth( context ) ){
		/* why not stack = DOWN_TO_PAGE( context->sp ) ? because if the program
		 * never uses all the stack space they requested then its wasted space.
		 * of course this might be inefficient if they suddenly use all that space
		 * and another memory trap has to occur but its better to save space
		 * in this memory starved kernel.
		 */
		current_process->process->stack = to_page( DOWN_TO_PAGE( context->addr ), 0 );
		if ( current_process->process->stack - to_page( VMEM_1_BASE, 0 ) < current_process->process->heap_end || 
		     grow_stack( current_process->process->page_table, current_process->process->stack - to_page( VMEM_1_BASE, 0 ) ) ){
			TracePrintf( 1, "Could not grow stack\n" );
			struct process_list * save = current_process->next;
			doExit( current_process->process, ERROR );
			current_process = skipIdle( save );
			switchTo( 0, current_process->process, context );
		} else {
			TracePrintf( 5, "[%d] Stack grew to page %d %p. heap top %p\n", current_process->process->id, current_process->process->stack, current_process->process->stack << PAGESHIFT, (void *)((current_process->process->heap_end << PAGESHIFT) + VMEM_1_BASE) );
		}
	} else {
		printf( "Invalid address %p. stack %p. Killing pid %d\n", context->addr, context->sp, current_process->process->id );
		struct process_list * save = current_process->next;
		doExit( current_process->process, ERROR );
		current_process = skipIdle( save );
		switchTo( 0, current_process->process, context );
	}
}

/* kill the process since it did an illegal math operation */
static void mathTrap( UserContext * context ){
	dummyTrap( "math", context );
	printf( "Process pid %d performed an illegal math operation\n", current_process->process->id );
	struct process_list * save = current_process->next;
	doExit( current_process->process, ERROR );
	current_process = skipIdle( save );
	switchTo( 0, current_process->process, context );
}

static void ttyReceiveTrap( UserContext * context ){
	int tty = context->code;
	int bytes = TtyReceive( tty, terminals[ tty ].input, TERMINAL_MAX_LINE );
	terminals[ tty ].bytes = bytes;
	/*
	terminals[ tty ].input[ bytes ] = 0;
	TracePrintf( 3, "Received %d bytes from terminal %d. %s\n", bytes, tty, terminals[ tty ].input );
	*/

	/* find the process that is waiting to read from this tty */
	struct process_list * list = findIOProcess( tty | TTY_READ );

	/* remove process from io list, put it back on the run queue */
	if ( list != 0 ){
		list->prev->next = list->next;
		if ( list->next ){
			list->next->prev = list->prev;
		}
		free( list->obj );
		list->obj = 0;
		list->next = current_process->next;
		list->prev = current_process;
		current_process->next->prev = list;
		current_process->next = list;
		swapProcesses( context );
	}
}

/*
static int countProcesses( struct process_list * list ){
	int count = 0;
	while ( list != 0 ){
		count += 1;
		list = list->next;
	}
	return count;
}
*/

/* a tty was written to so wake the process up that wrote to it.
 * Also wake up the next process waiting to write to the tty if it exists.
 * An interesting situation can occur if a process is waiting for the tty to
 * become free and the process that just wrote into the tty forks a child.
 * io1 = process that wrote to tty, about to wake up
 * io2 = process waiting for the tty to become free
 * io1child = process that io1 spawns
 *
 * So if io1 wakes up and immediately calls Fork() then io1child will be the next
 * process to run, not io2. If io1child then writes to this tty he will find tty
 * free, since io1 is done with it, and so io1child will write to the tty and wait
 * for his ttyTransmitTrap to occur. io2 in the mean time is runnable but when he wakes
 * up the tty will be suddenly busy again. This is ok because for io2 terminal->busy
 * will be 1 because io1child is using it so io2 will go back into the io list and
 * wait his turn. Obviously this can be a sort of denial of service for io2 if
 * io1child then spawns a child who does the same thing io1child did. Not that I
 * really care about such things since Fork() by itself could be used that way.
 */
static void ttyTransmitTrap( UserContext * context ){
	int tty = context->code;

	struct process_list * list = findIOProcess( tty | TTY_WRITE );
	int do_swap = 0;

	/* if a process is around then it was waiting for io to complete */
	if ( list != 0 ){
		TracePrintf( 3, "[%d] Wake up from tty write to terminal %d\n", list->process->id, tty );
		// TracePrintf( 3, "[%d] Processes in io list %d\n", list->process->id, countProcesses( io_list.next ) );
		list->prev->next = list->next;
		if ( list->next ){
			list->next->prev = list->prev;
		}
		free( list->obj );
		list->obj = 0;
		list->next = current_process->next;
		list->prev = current_process;
		current_process->next->prev = list;
		current_process->next = list;
		do_swap = 1;
	} else {
		panic( YALNIX_INVALID_PROGRAM );
	}

	/* find the next process waiting for the tty too. only one of them
	 * needs to be woken up, as opposed to all of them, since only one
	 * will succeed anyway.
	 */
	list = findIOProcess( tty | TTY_WRITE | TTY_WAIT );
	if ( list != 0 ){
		TracePrintf( 3, "[%d] Wake up waiting on tty write to terminal %d\n", list->process->id, tty );
		list->prev->next = list->next;
		if ( list->next ){
			list->next->prev = list->prev;
		}
		/* we actually don't have to free the object because the
		 * tty handler is going to use it again
		 */
		// free( list->obj );
		/* insert this object after the current process */
		struct process_list * after = current_process->next;
		list->next = after->next;
		list->prev = after;
		after->next->prev = list;
		after->next = list;
	}

	if ( do_swap ){
		swapProcesses( context );
	}
}

/* wake up the process that caused the disk trap to finish the transaction.
 * there should always be some process on the disk list if the disk trap
 * occurred.
 */
static void diskTrap( UserContext * context ){
	struct process_list * list = popDiskList();
	addToRunList( list );
	swapIfIdle( context );
}
/* end Yalnix traps */

static int setupTrapVector(){

	/* typedef makes our lives easier */
	typedef void (*trap)( UserContext * );

	/* all the traps */
	static trap traps[ TRAP_VECTOR_SIZE ];
	
	/* read the exception vector back out to ensure nothing bad happened */
	trap * table = NULL;

	/* clear traps array to ensure no bogus values get into the
	 * exception vector
	 */
	bzero( traps, sizeof(traps) );
	traps[ TRAP_KERNEL ] = kernelTrap;
	traps[ TRAP_CLOCK ] = clockTrap;
	traps[ TRAP_ILLEGAL ] = illegalTrap;
	traps[ TRAP_MEMORY ] = memoryTrap;
	traps[ TRAP_MATH ] = mathTrap;
	traps[ TRAP_TTY_RECEIVE ] = ttyReceiveTrap;
	traps[ TRAP_TTY_TRANSMIT ] = ttyTransmitTrap;
	traps[ TRAP_DISK ] = diskTrap;

	WriteRegister( REG_VECTOR_BASE, (unsigned int) traps );

	table = (trap *) ReadRegister( REG_VECTOR_BASE );
	/* make sure something got written */
	if ( table == NULL || table != traps ){
		return YALNIX_TRAP_SETUP_ERROR;
	}

	return 0;
}

/* the idle loop */
static void KernelIdleLoop( int id ){
	TracePrintf( 5, "Kernel idle loop\n" );
	while( 1 ){
		Pause();
	}
}

#if 0
static void testKernelHeap(){
	/*
	void * n = sbrk( 0 );
	while ( 1 ){
		printf( "Sbrk %p\n", n );
		n = sbrk( n ) + 0x1000;
	}
	*/
	int i;
	for ( i = 0; i < 500; i++ ){
		const int up = 0x4000;
		printf( "Malloc %p\n", malloc( up ) );
		// printf( "sbrk %p %d\n", i * up, sbrk( i * up ) );
	}
}
#endif

/*
KernelContext * identityKernelContext( KernelContext * context, void * k1, void * k2 ){
	dumpKernelContext( "in.context", context );
	dumpKernelContext( "k1.context", (KernelContext *) k1 );
	return context;
	// return (KernelContext *) k1;
}
*/


/* printf like syntax but you can leave off the %
 * this function adds arguments to a stack.
 */
void addArgsToStack( char * stack, int args_size, const char * format, ... ){
	va_list args;
	stack = stack - args_size + 4;
	TracePrintf( 7, "args stack %p\n", stack );
	va_start( args, format );
	for ( ; *format != 0; format++ ){
		switch( *format ){
			case '%' : break;
			case 'd' : {
				*(int *)stack = va_arg( args, int );
				stack += sizeof( int );
				break;
			}
			case 'c' : {
				/* char is promoted to int */
				*stack = va_arg( args, int );
				stack += sizeof( int );
				break;
			}
		}
	}
	va_end( args );
}

/* set up the idle process. give it one page of stack
 */
static struct process * createIdleProcess( int i ){
	struct process * idle = createProcess();
	if ( ! idle ){
		return 0;
	}

	/* only need one page of memory for the stack */
	idle->page_table[ idle->pages - 1 ] = get_free_page();
	idle->page_table[ idle->pages - 1 ]->virtual = to_page( VMEM_1_LIMIT, -1 ) - to_page( VMEM_1_BASE, 0 );
	idle->page_table[ idle->pages - 1 ]->protection = PROT_READ | PROT_WRITE;

	setupPageTable( idle->page_table, idle->pages );
	addArgsToStack( (void *)(VMEM_1_LIMIT - 4), sizeof(int) * 1, "%d", i );
	idle->user_context.sp = (void *)(VMEM_1_LIMIT - sizeof(int) * 1 - 4);
	idle->user_context.pc = KernelIdleLoop;

	return idle;
}

/*
KernelContext * swapKernel( KernelContext *kContext, void * from_pcb, void * to_pcb ){
	struct process * from = (struct process *) from_pcb;
	setupKernelStack( from->kernel_stack, KERNEL_STACK_PAGES );
	TracePrintf( 2, "Swapped kernel stuff\n" );
	return &from->kernel_context;
}
*/

static void initializeTerminals(){
	int i;
	for ( i = 0; i < NUM_TERMINALS; i++ ){
		terminals[ i ].bytes = -1;
		terminals[ i ].busy = 0;
	}
}

static void loadCommandLinePrograms( char ** args ){
	YalnixError ret;
	/* load all programs given on the command line */
	for ( ; *args != 0; args++ ){
		char * buf[ 2 ];
		struct process * program = createProcess();
		if ( ! program ){
			continue;
		}
		struct process_list * list = (struct process_list *) malloc( sizeof( struct process_list ) );
		if ( ! list ){
			freeProcess( program );
			continue;;
		} else {
			list->process = program;
			program->list = list;
		}

		buf[ 0 ] = *args;
		buf[ 1 ] = 0;
		ret = loadProgram( program, *args, buf );
		if ( ret == ERROR || ret == YALNIX_INVALID_PROGRAM ){
			printf( "[kernel] Could not load %s\n", *args );
			freeProcess( program );
			free( list );
		} else {
			addToRunList( list );
		}
	}
}

/* main kernel entry */
void KernelStart( char ** args, unsigned int memorySize, UserContext * context ){
	YalnixError ret = 0;
	printf( "[kernel] Hello Yalnix\n" );
	
	initializeVirtualMemory( memorySize );

	ret = setupTrapVector();
	if ( ret != 0 ){
		panic( ret );
	}

	initializeTerminals();

	/* original kernel context for new processes */
	static KernelContext kernel_context;
	/* original kernel stack for new processes */
	static char * stack;
	static int setup = 0;
	/* it would be nice to use get_pages(2) so that I can use copyPage later on */
	stack = malloc( PAGESIZE * 2 );
	if ( ! stack ){
		panic( YALNIX_OUT_OF_MEMORY );
	}
	bzero( &kernel_context, sizeof(kernel_context) );
	bzero( stack, PAGESIZE * 2 );

	// testKernelHeap();

	struct process * idle = createIdleProcess( 1 );

	/* the first process must be idle */
	setIdleProcess( idle );
	// run_list.process = idle;

	loadCommandLinePrograms( args );

	setup = 0;

	/* each process will start from here when it is first context switched to life */
	getCurrentKernelContext( &kernel_context, stack );

	/* only set the initial kernel context and stack one time before the very first
	 * process starts running.
	 */
	if ( !setup ){
		struct process_list * list = getRunList();
		setup = 1;
		do{
			struct process * p = list->process;
			p->kernel_context = kernel_context;
			TracePrintf( 5, "Copy stack to pid %d\n", p->id );
			ret = copyStack( p->kernel_stack, stack );
			if ( ret != YALNIX_NO_ERROR ){
				panic( ret );
			}
			list = list->next;
		} while ( list != getRunList() );
		free( stack );
		switchTo( 0, current_process->process, context );
	}
	TracePrintf( 8, "First switch to process %d\n", current_process->process->id );
	*context = current_process->process->user_context;
	setupPageTable( current_process->process->page_table, current_process->process->pages );

	return;
}

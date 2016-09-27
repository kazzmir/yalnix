#ifndef _yalnix_process_h
#define _yalnix_process_h

#include "hardware.h"
#include "yalnix.h"
#include "memory.h"

/* number of kernel stack pages */
#define KERNEL_STACK_PAGES ((KERNEL_STACK_LIMIT - KERNEL_STACK_BASE) / PAGESIZE)

/* possible process states */
#define PROCESS_RUNNABLE 1
#define PROCESS_DIED 2
#define PROCESS_WAIT_FOR_DEAD_CHILD 3

#define IPC_MAX_LENGTH 32

/* used for the termination list each process has to keep track of
 * dead children.
 */
struct status_list{
	/* exit code of child */
	int status;
	/* pid of child */
	int id;
	/* linked list pointer */
	struct status_list * next;
};

struct process;

/* linked list of processes */
struct process_list{
	struct process * process;
	struct process_list * next;
	struct process_list * prev;

	/* can keep track of arbitrary objects for things like delay counters.
	 * basically obj is a polymorphic object whose type is deduced by what
	 * process queue this process_list is on.
	 */
	void * obj;
};

/* The PCB in other lingo's */
struct process{

	/* 2 pages of memory for the kernel */
	struct memory_page * kernel_stack[ KERNEL_STACK_PAGES ];

	/* process id */
	int id;

	/* running/waiting/died/whatever */
	int status;

	/* total number of pages in region 1 */
	int pages;

	/* current page the bottom of the stack is at */
	int stack;

	/* which process list this process is on */
	struct process_list * list;

	/* top of the heap. addresses below this value are accessible */
	int heap_end;
	/* bottom of the heap */
	int heap_start;

	/* parent process */
	struct process * parent;

	/* array of alive children */
	struct process ** children;

	/* size of children array */
	int max_children;

	/* linked list of status's for dead children */
	struct status_list terminated;

	/* pages in region 1 that this process owns */
	struct memory_page ** page_table;
	
	/* messages sent to this process wait here */
	caddr_t inbox[ IPC_MAX_LENGTH ];

	/* registers for user space */
	UserContext user_context;

	/* registers for kernel space */
	KernelContext kernel_context;
};

struct process * createProcess(void);
void freeProcess( struct process * process );
int addChildToProcess( struct process * parent, struct process * child );

#endif

#include <stdlib.h>

#include "schedule.h"
#include "kernel.h"
#include "yalnix.h"
#include "process.h"

/* circular list of processes that can run */
static struct process_list run_list = { .process = 0, .next = &run_list, .prev = &run_list };
/* linked list of processes that are waiting for something, like dead children */
static struct process_list busy_list = { .process = 0, .next = 0 };
/* linked list of processes waiting on i/o */
static struct process_list io_list = { .process = 0, .next = 0 };
/* linked list of processes that called the delay syscall */
static struct process_list delayed_list = { .process = 0, .prev = 0, .next = 0, .obj = 0 };
/* linked list of processes waiting on some form of ipc */
static struct process_list ipc_list = { .process = 0, .prev = 0, .next = 0, .obj = 0 };

static struct process_list disk_list = { .process = 0, .prev = 0, .next = 0, .obj = 0 };

struct process_list * current_process = &run_list;

/* if list is the idle process return the next process available,
 * otherwise just return list as-is.
 */
struct process_list * skipIdle( struct process_list * list ){
	if ( list == &run_list ){
		return list->next;
	}
	return list;
}

int isIdle( struct process_list * list ){
	return list == &run_list;
}
	
void setIdleProcess( struct process * idle ){
	run_list.process = idle;
}

struct process_list * getRunList(){
	return &run_list;
}

/* returns the next process in the run list. if the next process is the idle
 * process, &run_list, then try to skip over it to the next process. in the event
 * that there is only one process in the run_list, that being the idle process,
 * then the second list = list->next amounts to a nop
 */
static struct process_list * nextProcess( struct process_list * list ){
	return skipIdle( list->next );
}

/* put the current process in old and the next process in new */
void getNextProcess( struct process ** old, struct process ** new ){
	
	/* kill the os if there are no processes left */
	if ( run_list.next == &run_list &&
	     io_list.next == 0 &&
	     busy_list.next == 0 &&
	     ipc_list.next == 0 &&
	     delayed_list.next == 0 ){
		Halt();
	}

	*old = current_process->process;
	current_process = nextProcess( current_process );
	*new = current_process->process;
}

static void addToListFront( struct process_list * head, struct process_list * add ){
	add->next = head->next;
	add->prev = head;
	if ( add->next ){
		add->next->prev = add;
	}
	head->next = add;
}

static void addToListTail( struct process_list * head, struct process_list * list ){
	while ( head->next != 0 ){
		head = head->next;
	}

	list->next = 0;
	head->next = list;
	list->prev = head;
}

void addToIpcList( struct process_list * list ){
	addToListFront( &ipc_list, list );
}

void addToBusyList( struct process_list * list ){
	addToListFront( &busy_list, list );
}

void addToDelayedList( struct process_list * list ){
	addToListFront( &delayed_list, list );
}

void addToDiskList( struct process_list * list ){
	addToListTail( &disk_list, list );
}

struct process_list * popDiskList(){
	struct process_list * list = disk_list.next;
	disk_list.next = disk_list.next->next;
	if ( disk_list.next ){
		disk_list.next->prev = &disk_list;
	}
	return list;
}

void addToIoList( struct process_list * list ){
	addToListTail( &io_list, list );
}

struct process_list * getDelayedList(){
	return delayed_list.next;
}

/* search a linked list for a process with the given id.
 * this handles circular lists.
 */
static struct process * findProcessInList( struct process_list * list, int id ){
	struct process_list * head = list;
	while ( list != 0 && list->process->id != id ){
		list = list->next;
		if ( list == head ){
			break;
		}
	}

	if ( list != 0 && list->process->id == id ){
		return list->process;
	}
	return 0;
}

/* search the system for the process with the given id */
struct process * findProcess( int id ){
	/* quick optimization */
	if ( id == current_process->process->id ){
		return current_process->process;
	}

	struct process_list * lists[] = { &run_list, busy_list.next,
	                                  io_list.next, delayed_list.next,
					  ipc_list.next };
	int i = 0;
	for ( i = 0; i < sizeof(lists) / sizeof(struct process_list *); i++ ){
		struct process * found = findProcessInList( lists[ i ], id );
		if ( found != 0 ){
			return found;
		}
	}
	return 0;
}

/* add a process to the list of running processes */
void addToRunList( struct process_list * list ){
	list->next = current_process->next;
	list->prev = current_process;
	if ( list->next ){
		list->next->prev = list;
	}
	current_process->next = list;
}

/* find a process in the io list that has obj equal to attribute
 */
struct process_list * findIOProcess( int attribute ){
	struct process_list * list = io_list.next;
	while ( list != 0 && *((int *) list->obj) != attribute ){
		list = list->next;
	}
	return list;
}

/* find a process that matches some combination of to/from/receive */
struct process_list * findIpc( int to, int from, int receive ){
	struct process_list * list = ipc_list.next;

	while ( list != 0 ){
		struct ipc * obj = (struct ipc *) list->obj;
		/* looking for someone sending to 'to' */
		if ( to != -1 ){
			if ( from == -1 ){
				if ( obj->to == to ){
					return list;
				}
			} else {
				/* looking for a specific sender */
				if ( obj->to == to &&
				     obj->from == from ){
					return list;
				}
			}
		/* otherwise look for someone receiving with id 'receive' */
		} else if ( receive != -1 ){
			if ( from == -1 ){
				if ( obj->receive == receive ){
					return list;
				}
			} else {
				/* look for a receiver asking for a specific sender */
				if ( obj->receive == receive &&
				     obj->from == from ){
					return list;
				}
			}
		}
		list = list->next;
	}

	return 0;
}

void removeProcess( struct process * process ){

	struct process_list * list = process->list;
	if ( list->prev ){
		list->prev->next = list->next;
	}
	if ( list->next ){
		list->next->prev = list->prev;
	}
	free( list );
	freeProcess( process );
}

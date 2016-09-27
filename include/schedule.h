#ifndef _yalnix_schedule_h
#define _yalnix_schedule_h

#include "process.h"

extern struct process_list * current_process;

struct process_list * skipIdle( struct process_list * list );
void getNextProcess( struct process ** old, struct process ** new );
void setIdleProcess( struct process * idle );

void addToBusyList( struct process_list * list );
void addToDelayedList( struct process_list * list );
void addToIoList( struct process_list * list );
void addToIpcList( struct process_list * list );
void addToRunList( struct process_list * list );
void addToDiskList( struct process_list * list );

int isIdle( struct process_list * list );

struct process_list * findIpc( int to, int from, int receive );
struct process * findProcess( int id );
struct process_list * findIOProcess( int attribute );

struct process_list * getDelayedList();
struct process_list * getRunList();

struct process_list * popDiskList();

void removeProcess( struct process * process );

#endif

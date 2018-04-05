#ifndef __SHARED_STRUCTS_H__
#define __SHARED_STRUCTS_H__
#include "realtime.h"
#include "3140_concur.h"

/** Implement your structs here */

/**
 * This structure holds the process structure information
 */

typedef struct process_state{
	unsigned int * initial_sp; // Used in Process Reinit for Periodic Proceses
	unsigned int * sp; // Stack Pointer of Process
	unsigned int stack_size; // Used in initialization of stack
	unsigned int f; // function of process. Used in Process Reinit
	int ready; // Indicates whether process is ready or blocked
	process_t* next; // Next process of process queue
	realtime_t* startTime; //starttime of process
	realtime_t* deadline; //deadline of process
	realtime_t* period; //period of process
	int priority; //Priority of processes. Due to change via Priority Inversion Protocols
	int initial_priority; //Initial priority of processes
} process_t;

/**
 * This defines the lock structure
 */
typedef struct lock_state {
	process_t* block_list; // start of the list of blocked processes
	process_t* locked_process; //process currently holding the lock
	int lock_held; // whether or not the lock is held; 0 means the lock is free
	int highest_process; //highest priority of all processes holding the lock. This must be computed offline.
} lock_t;

#endif

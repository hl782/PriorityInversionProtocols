#include "3140_concur.h"
#include "realtime.h"
#include "shared_structs.h"
#include "fsl_debug_console.h"
#include <limits.h>
#include <stdlib.h>
#include <fsl_device_registers.h>

process_t* current_process;
process_t* head;
process_t* rt_start_queue_head;
process_t* rt_wait_queue_head;

//0 = No Protocol; 1 = NPP; 2 = HLP; 3 = PIP;
int priority_protocol = 1;
int process_deadline_met = 0;
int process_deadline_miss = 0;

uint32_t saved_mask;

realtime_t currentTime;
realtime_t currentTime = {0,0}; //initialize the currentTime

//Returns true if t1 <= t2
//This will be to check if our new realtime task needs to be come our head
int time_lte(realtime_t* t1, realtime_t* t2){
	return ((((t1->sec)*1000) + (t1->msec)) <= (((t2->sec)*1000) + (t2->msec)));
}

//Returns true if t1 < t2
int time_lt(realtime_t* t1, realtime_t* t2){
	return ((((t1->sec)*1000) + (t1->msec)) < (((t2->sec)*1000) + (t2->msec)));
}

//Adds up two realtimes
int add_time(realtime_t* t1, realtime_t* t2){
	return ((t1->sec)*1000 + t1->msec + (t2->sec)*1000 + t2->msec);
}

//Adds process to top of realtime start queue
//Sorted by priority
void push_rt_start_queue(process_t* new_p){
	process_t* tmp;
	process_t* prev;
	if(new_p == NULL){
		return;
	}
	if(rt_start_queue_head == NULL){
		rt_start_queue_head = new_p;
		new_p->next = NULL;
	}else{
		if(rt_start_queue_head->priority <= new_p->priority){
			new_p->next = rt_start_queue_head;
			rt_start_queue_head = new_p;
		}else{
			tmp = rt_start_queue_head;
			while(tmp != NULL && tmp->priority > new_p->priority){
				prev = tmp;
				tmp = tmp->next;
			}
			prev->next = new_p;
			new_p->next = tmp;
		}
	}
	
}

//Pops the head of the realtime start queue
process_t* pop_rt_start_queue(){
	process_t* ret;
	if(rt_start_queue_head == NULL){
		return NULL;
	}
	ret = rt_start_queue_head;
	rt_start_queue_head = rt_start_queue_head->next;
	return ret;
}


//Remove a specific element of the realtime start queue
void rem_rt_start_queue(int initial_priority){
	process_t* prev;
	process_t* temp;
	if(rt_start_queue_head == NULL){
		return;
	}
	if(rt_start_queue_head->initial_priority == initial_priority){
		rt_start_queue_head = rt_start_queue_head->next;
	}else{
		temp = rt_start_queue_head;
		while(temp->next != NULL && temp->initial_priority != initial_priority){
			prev = temp;
			temp = temp->next;
		}
		prev->next = temp->next;
	}
}



//Adds process to top of realtime wait queue
//Sorted by earliest startTime
void push_rt_wait_queue(process_t* new_p){
	process_t* tmp;
	process_t* prev;
	if(new_p == NULL){
		return;
	}
	if(rt_wait_queue_head == NULL){
		rt_wait_queue_head = new_p;
		new_p->next = NULL;
	}else{
		if(time_lte(new_p->startTime, rt_wait_queue_head->startTime)){
			new_p->next = rt_wait_queue_head;
			rt_wait_queue_head = new_p;
		}else{
			tmp = rt_wait_queue_head;
			while(tmp != NULL && time_lte(tmp->startTime, new_p->startTime)){
				prev = tmp;
				tmp = tmp->next;
			}
			prev->next = new_p;
			new_p->next = tmp;
		}
	}
}



//Pops the head of the realtime wait queue
process_t* pop_rt_wait_queue(){
	process_t* ret;
	if(rt_wait_queue_head == NULL){
		return NULL;
	}
	ret = rt_wait_queue_head;
	rt_wait_queue_head = rt_wait_queue_head->next;
	return ret;
}

/*----------------------------------------------------------------------------
  Initialize the lock structure
*----------------------------------------------------------------------------*/
void l_init(lock_t* l) {
	l->block_list = NULL; // No blocked processes for this lock (initially)
	l->lock_held = 0;	// lock is not held
	l->locked_process = NULL; // No process is holding this lock (initially)
}
 
/*----------------------------------------------------------------------------
  Grab the lock or block the process until lock is available
*----------------------------------------------------------------------------*/
void l_lock(lock_t* l) {
	PIT->CHANNEL[0].TCTRL = 0x1; // disable interrupts
	if (l->lock_held) {	// block process and add it to block queue for this lock
		current_process->ready = 0;
		process_t *tmp;
		if (l->block_list == NULL) {
			l->block_list = current_process;
			current_process->next = NULL;			
		}
		else {
			//Sorts blocked list by priority order
			if(l->block_list->priority <= current_process->priority){
				current_process->next = l->block_list;
				l->block_list = current_process;
			}else{
				process_t* prev;
				tmp = l->block_list;
				while(tmp != NULL && tmp->priority > current_process-> priority){
					prev = tmp;
					tmp = tmp->next;
				}
				prev->next = current_process;
				current_process->next = tmp;
			}
		}
		if(priority_protocol == 3){
			int max = l->block_list->priority;
			process_t* tmp = l->block_list;
			while(tmp != NULL){			
				if(tmp->priority > max){
					max = tmp->priority;
				}
				tmp = tmp->next;
			}
			rem_rt_start_queue(l->locked_process->initial_priority);
			l->locked_process->priority = max;
			push_rt_start_queue(l->locked_process);
		}
		PIT->CHANNEL[0].TCTRL = 0x3; //s enable interrupts
		process_blocked();
	}
	else {	// otherwise acquire lock
		l->lock_held = 1;
		l->locked_process = current_process;
		if(priority_protocol == 1){
			current_process->priority = INT_MAX;
		}else if(priority_protocol == 2){
			current_process->priority = l-> highest_process;
		}
	}
	PIT->CHANNEL[0].TCTRL = 0x3; // enable interrupts
}
 
/*----------------------------------------------------------------------------
  Unlock Function
*----------------------------------------------------------------------------*/
void l_unlock(lock_t* l) {
	PIT->CHANNEL[0].TCTRL = 0x1; // disable interrupts
	l->locked_process = NULL;
	if(priority_protocol == 1 || priority_protocol == 2 || priority_protocol == 3){
		current_process->priority = current_process->initial_priority;
	}
	l->lock_held = 0;
	process_t* unblock = NULL;
	if(l->block_list){
		unblock = l->block_list;
		unblock->ready = 1;
		l->block_list = unblock->next;
		
		if(time_lt(&currentTime, unblock->startTime)){
			push_rt_wait_queue(unblock);
		}else{
			push_rt_start_queue(unblock);
		}			
	}
	PIT->CHANNEL[0].TCTRL = 0x3; // enable interrupts
}

//creates a new realtime process and adds to the respective linked list

int process_rt_periodic_create(void (*f)(void), int n, realtime_t* start, realtime_t* deadline, realtime_t* period, int priority, int initial_priority){
	unsigned int* new_stack = process_stack_init(*f, n);
	process_t* new_process = (process_t *) malloc(sizeof(process_t));
	if(new_process == NULL || new_stack == NULL){
		return -1;
	}
	
	new_process->initial_sp = new_stack;
	new_process->sp = new_stack;
	new_process->stack_size = n;
	new_process->f = (unsigned int) f;
	new_process->ready = 1;
	new_process->startTime = start;
	new_process->deadline = deadline;
	new_process->period = period;
	new_process->priority = priority;
	new_process->initial_priority = initial_priority;

	if(time_lt(&currentTime, new_process->startTime)){
		push_rt_wait_queue(new_process);
	}else{
		push_rt_start_queue(new_process);
	}
	return 0;
}

void process_stack_reinit (process_t * proc)
{
	int sum = add_time(proc->startTime, proc->period);
	proc->startTime->sec = sum/1000;
	proc->startTime->msec = sum%1000;
		
	int n = 18;
	unsigned int *tmp_sp = proc->initial_sp;
	tmp_sp[n-1] = 0x01000000;
	tmp_sp[n-2] = proc->f;
	tmp_sp[n-3] = (unsigned int) process_terminated;
	tmp_sp[n-9] = 0xFFFFFFF9;
	tmp_sp[n-18] = 0x3;
	
	proc->sp = proc->initial_sp;
	proc->priority = proc->initial_priority;
}

void process_start (void){
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
	PIT->MCR = 0;
	PIT->CHANNEL[0].LDVAL = 1200000/5; //Schedule Timer
	NVIC_EnableIRQ(PIT0_IRQn);
	PIT->CHANNEL[1].LDVAL = 120000/2; //Millisecond of delay
	PIT->CHANNEL[1].TCTRL = 3;
	NVIC_EnableIRQ(PIT1_IRQn);
	NVIC_SetPriority(SVCall_IRQn, 1);
	NVIC_SetPriority(PIT0_IRQn, 2);
	NVIC_SetPriority(PIT1_IRQn, 0);
	process_begin();
}

unsigned int * process_select(unsigned int * cursp){
	//If there is a process running
	if(cursp != NULL){
		current_process->sp = cursp;
		//if the process isnt blocked
		if (current_process->ready){
			push_rt_start_queue(current_process);
			current_process = NULL;
		}
	}else{ //no more running processes
		if(current_process != NULL){
			//if the current process has a deadline, and it is later than our current time, increment miss-count, if not increment met-count.
			if(current_process->deadline != NULL){
				if(add_time(current_process->startTime, current_process->deadline) < (currentTime.sec*1000+currentTime.msec)){
					process_deadline_miss++;
				}else{
					process_deadline_met++;
				}
			}			
			
			//free the process and stack if it is a nonperiodic process
			if (current_process->period == NULL){
				free(current_process);
				process_stack_free(current_process->sp, current_process->stack_size);
			}
			//Otherwise, reinitialize the process and send to realtime wait queue
			else{
				process_stack_reinit(current_process);
				push_rt_wait_queue(current_process);
				current_process = NULL;							
			}
		}
	}	
	
	//Check if there are processes in the wait queue that can be moved to start queue
	while(rt_wait_queue_head != NULL && time_lte(rt_wait_queue_head->startTime, &currentTime)){ 
			process_t* pop = pop_rt_wait_queue();
			push_rt_start_queue(pop);
			current_process = NULL;
	}
	
	//Order for picking the next process
	//First check if there are any realtime processes that can be run
	if(rt_start_queue_head != NULL){
		current_process = pop_rt_start_queue();
		return current_process->sp;
	}else if(rt_wait_queue_head != NULL){ //if both queues are empty and there's still pending RT processes
		while(rt_start_queue_head == NULL){ //busywait until currentTime passes startTime of rt_wait_queue_head
			if(time_lte(rt_wait_queue_head->startTime, &currentTime)){ 
				process_t* pop = pop_rt_wait_queue();
				push_rt_start_queue(pop);
				current_process = NULL;
			}
		}
		current_process = pop_rt_start_queue();
		return current_process->sp;
	}else{ //otherwise no more processes left at all
		return 0;
	}
}

void PIT1_IRQHandler(void){
	if (currentTime.msec == 999) {
      currentTime.sec = currentTime.sec + 1;
      currentTime.msec = 0;
	} else {
	  currentTime.msec = currentTime.msec + 1;
	}
	PIT->CHANNEL[1].TCTRL = 0;
  	PIT->CHANNEL[1].TFLG |= PIT_TFLG_TIF_MASK;
  	PIT->CHANNEL[1].TCTRL|= 3;
	return;
}

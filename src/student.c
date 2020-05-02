/*
 * student.c
 * Multithreaded OS Simulation for CS 2200
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "student.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* Function prototypes */
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);



/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 *
 * ready is a pointer to a struct you should use for your ready queue
 * implementation. The head of the queue corresponds to the process
 * that is about to be scheduled onto the CPU, and the tail is for
 * convenience in the enqueue function. See student.h for the 
 * relevant function and struct declarations.
 *
 * Similar to current[], ready is accessed by multiple threads,
 * so you will need to use a mutex to protect it. ready_mutex has been
 * provided for that purpose.
 *
 * The condition variable queue_not_empty has been provided for you
 * to use in conditional waits and signals.
 *
 * Please look up documentation on how to properly use pthread_mutex_t
 * and pthread_cond_t.
 */
static pcb_t **current;
static queue_t *ready;
static pthread_mutex_t current_mutex;
static pthread_mutex_t ready_mutex;
static pthread_cond_t queue_not_empty;

static int timeslice;
static int cpu_num;


typedef enum {
    FIFO,
    RoundRobin,
    Priority
} scheduler;

static scheduler schedule_type;
/*
 * enqueue() is a helper function to add a process to the ready queue.
 *
 * This function should add the process to the ready queue at the
 * appropriate location.
 *
 * NOTE: For priority scheduling, you will need to have additional logic
 * in either this function or the dequeue function to make the ready queue
 * a priority queue.
 */
void enqueue(queue_t *queue, pcb_t *process)
{
    /* FIX ME */
	if (schedule_type == Priority) {
       process -> state = PROCESS_READY;
       pthread_mutex_lock(&ready_mutex);
       if (queue -> head == NULL) {
           queue -> head = process;
           queue -> tail = process;
       } else {
           unsigned int added_priority = process -> priority;
           pcb_t* curr = queue -> head;
           //check if should add to head
           if (added_priority < curr -> priority) {
               process -> next = queue -> head;
               queue -> head = process;
           } else {//not adding before head, have at least one element
               while (curr -> next != NULL && (added_priority < (curr -> next -> priority))) {
                   curr = curr -> next;
               }
               //check if it's tail
               if (curr -> next == NULL) {//tail
                   queue -> tail -> next = process;
                   queue -> tail = process;
               } else {//in the middle
                   process -> next = curr -> next;
                   curr -> next = process;
               }
           }
       }
       pthread_cond_signal(&queue_not_empty);
       pthread_mutex_unlock(&ready_mutex);

	} else {
		pthread_mutex_lock(&ready_mutex);
        if (queue -> head == NULL) {
            queue -> head = process;
            queue -> tail = process;
        } else {
            queue -> tail -> next = process;
            queue -> tail = process;
        }
        pthread_cond_signal(&queue_not_empty);
        pthread_mutex_unlock(&ready_mutex);
	}
}

/*
 * dequeue() is a helper function to remove a process to the ready queue.
 *
 * This function should remove the process at the head of the ready queue
 * and return a pointer to that process. If the queue is empty, simply return
 * NULL.
 *
 * NOTE: For priority scheduling, you will need to have additional logic
 * in either this function or the enqueue function to make the ready queue
 * a priority queue.
 */
pcb_t* dequeue(queue_t *queue)
{
    /* FIX ME */
	if (queue == NULL || queue -> head == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&ready_mutex);
    pcb_t* deleted_node = queue -> head;
    if (queue -> head -> next == NULL) {//only one element
        queue -> head = NULL;
        queue -> tail = NULL;
    } else {
    	queue -> head = queue -> head -> next;
    }
    pthread_mutex_unlock(&ready_mutex);
    deleted_node -> next = NULL;
    deleted_node -> state = PROCESS_RUNNING;
    return deleted_node;
}

/*
 * is_empty() is a helper function that returns whether the ready queue
 * has any processes in it.
 *
 * If the ready queue has no processes in it, is_empty() should return true.
 * Otherwise, return false.
 */
bool is_empty(queue_t *queue)
{
    /* FIX ME */
	pthread_mutex_lock(&ready_mutex);

	if (queue->head == NULL) {
		pthread_mutex_unlock(&ready_mutex);
		return true;
	}

	pthread_mutex_unlock(&ready_mutex);
    return false;
}

/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Set the currently running process using the current array
 *
 *   4. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *
 *	The current array (see above) is how you access the currently running process indexed by the cpu id.
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */

	if (is_empty(ready)) {
		context_switch(cpu_id, NULL, timeslice);
	} else {
		pcb_t* current_process = dequeue(ready);
		pthread_mutex_lock(&current_mutex);
		current[cpu_id] = current_process;
		pthread_mutex_unlock(&current_mutex);
		context_switch(cpu_id, current_process, timeslice);
	}





	// pcb_t* current_process  = dequeue(ready);

 //    if (current_process != NULL) {//ready queue not empty
 //        pthread_mutex_lock(&current_mutex);
 //        current_process -> state = PROCESS_RUNNING;

 //        current[cpu_id] = current_process;
 //        pthread_mutex_unlock(&current_mutex);
 //    }
 //    context_switch(cpu_id, current_process, timeslice);//!!! the time slice will change
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    /* FIX ME */
	pthread_mutex_lock(&ready_mutex);
        while (ready -> head == NULL) {
            pthread_cond_wait(&queue_not_empty, &ready_mutex);
        }
        pthread_cond_signal(&queue_not_empty);//??????should I have this?
        pthread_mutex_unlock(&ready_mutex);
        schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
    //mt_safe_usleep(1000000);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring in the case of Round Robin
 * scheduling or if a process with a higher priority is ready in the case of
 * Priority with Preemption scheduling.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 *
 * Remember to set the status of the process to the proper value.
 */
extern void preempt(unsigned int cpu_id)
{
    /* FIX ME */
	pthread_mutex_lock(&current_mutex);
    pcb_t* current_process = current[cpu_id];
    current_process -> state = PROCESS_READY;
    enqueue(ready, current_process);
    pthread_mutex_unlock(&current_mutex);

    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    /* FIX ME */
	pthread_mutex_lock(&current_mutex);
    pcb_t* current_process = current[cpu_id];
    current_process -> state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);

    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    /* FIX ME */
	pthread_mutex_lock(&current_mutex);
	current[cpu_id]->state = PROCESS_TERMINATED;
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is Priority with Preemption, wake_up() may need
 *      to preempt the process that is currently executing on the CPU to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with an equal or higher priority than the one which just woke up,
 *      wake_up() should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    /* FIX ME */

	process -> state = PROCESS_READY;
  enqueue(ready, process);
	

	if (schedule_type == Priority) {
       //1. will use force_preempt() when
           //1. NOT: idle()
           //2. NOT: all priority higher than *process
       bool running_idle = false;
       unsigned int biggest_priority = 0;
       unsigned int biggest_priority_cpu = 0;
       int i;
       pthread_mutex_lock(&current_mutex);
       for (i = 0; i < cpu_num && !running_idle; i++) {

           if (current[i] == NULL) {
               running_idle = true;
           } else if (current[i] -> priority > biggest_priority) {//update biggest priority
               biggest_priority = current[i] -> priority;
               biggest_priority_cpu = (unsigned int)i;
           }

       }
       pthread_mutex_unlock(&current_mutex);
       if (biggest_priority > process -> priority) {
           force_preempt(biggest_priority_cpu);
       }
   }
}



/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
    unsigned int cpu_count;

    /*
     * Check here if the number of arguments provided is valid.
     * You will need to modify this when you add more arguments.
     */
    if (argc < 2 || argc > 4)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -s ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Priority with Preemption Scheduler\n\n");
        return -1;
    }

    /* Parse the command line arguments */
    cpu_count = strtoul(argv[1], NULL, 0);
	cpu_num = (int)cpu_count;

    /* FIX ME - Add support for -r and -s parameters*/
	if (argc == 2) {
		schedule_type = FIFO;
		timeslice = -1;
	} else if (argv[2][1] == 'r') {
		schedule_type = RoundRobin;
		timeslice = atoi(argv[3]);
	} else if (argv[2][1] == 'p') {
		schedule_type = Priority;
		timeslice = -1;
	}

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    /* Allocate the ready queue struct and its mutex */
    ready = malloc(sizeof(queue_t));
    assert(ready != NULL);
    pthread_mutex_init(&ready_mutex, NULL);

    /* Initialize the condition variable */
    pthread_cond_init(&queue_not_empty, 0);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


#pragma GCC diagnostic pop

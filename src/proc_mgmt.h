/* 
 * File:   proc_mgmt.h
 * Author: yin
 *
 * Created on September 22, 2013, 5:31 PM
 */

#ifndef PROC_MGMT_H
#define	PROC_MGMT_H

#include "base/global.h"
#include "data_struct.h"
#include "storage_mgmt.h"

typedef struct process
{
    INT32 pid;
    INT32 priority;
    INT32 delay_time;
    void *context;
    void *entry_point;
    BOOL suspend;
    BOOL need_message;
    INT16 disk_id;
    INT16 operation;
    INT32 disk;
    INT32 sector;
    DISK_DATA *disk_data;
    char process_name[MAX_NUMBER_OF_PROCESSE_NAME + 1];
} PCB;

typedef struct message
{
    long target_pid;
    long source_pid;
    long send_length;
    char message_buffer[MAX_LENGTH_OF_LEGAL_MESSAGE + 1];
} MSG;

// Description: Check if the two processes match.
// Parameter @data1 & @data2: Two processes are to be checked.
// Return: 1 indicates matching.  0 indicates does not match.
int match_pcb(const void *data1, const void *data2);

// Description: Check if the two process IDs match.
// Parameter @data1: The process whose ID to be compared.
// Parameter @pid: The processes ID to be compared.
// Return: 1 indicates matching.  0 indicates does not match.
int match_pid(const void *data1, const void *pid);

// Description: Compare the delay time of two processes.
// Parameter @data1 & @data2: Two processes whose delay time are to be compared.
// Return: return an integer less than, equal to, or greater than zero
// if the delay time of data1 is found, respectively, to be less than,
// to match, or be greater than that of data2.
int compare_time(const void *data1, const void *data2);

// Description: Compare the priority of two processes.
// Parameter @data1 & @data2: Two processes whose priority are to be compared.
// Return: return an integer less than, equal to, or greater than zero
// if the priority of data1 is found, respectively, to be less than,
// to match, or be greater than that of data2.
int compare_priority(const void *data1, const void *data2);

// Description: Initialize the global process table.
// Parameter: None.
// Return: None.
void init_process_table();

// Description: Create and initialize several queues.
// Parameter: None.
// Return: On success, 1 is returned.  On error, 0 is returned.
int init_queues();

// Description: Add a process to the global process table.
// Parameter @pcb_to_add: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.
int add_to_process_table(PCB *pcb_to_add);

// Description: Remove a process from the global process table.
// Parameter @pcb_to_remove: The process to remove.
// Return: On success, 1 is returned.  On error, 0 is returned.
int remove_from_process_table(PCB *pcb_to_remove);

// Description: Print scheduling information by using scheduler printer.
// Parameter @action_mode: The string indicates the action mode.
// Parameter @target_pcb: The process on which the scheduler action is being performed.
// Parameter @info_type: indicate if print the final end information.
// Return: None.
void print_scheduling_info(char *action_mode, PCB *target_pcb, int info_type);

// Used for debugging, print information of all processes in the process table.
void print_process_table(void);

// Used for debugging, print information of the given process.
void print_pcb(PCB *pcb);

// Used for debugging, print information of all processes in the given queue.
void print_queue(Queue *queue);

// Description: Add a process to the ready queue.
// Parameter @pcb: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.
int add_to_ready_queue(PCB *pcb);

// Description: Remove a process from the ready queue.
// Parameter @pcb: The process returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int remove_from_ready_queue(PCB **pcb);

// Description: Dequeue a process from the ready queue at the head.
// Parameter @pcb: The process returned from the dequeued element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int dequeue_from_ready_queue(PCB **pcb);

// Description: Add a process to the timer queue.
// Parameter @pcb: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.
int add_to_timer_queue(PCB *pcb);

// Description: Remove a process from the timer queue.
// Parameter @pcb: The process returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int remove_from_timer_queue(PCB **pcb);

// Description: Dequeue a process from the timer queue at the head.
// Parameter @pcb: The process returned from the dequeued element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int dequeue_from_timer_queue(PCB **pcb);

// Description: Find an element in a queue according to specific condition.
// Parameter @queue: The queue where the pid will be found.
// Parameter @fp_match: The callback used to match data in the element.
// Parameter @data: The condition must be matched.
// Return: On success, the pointer of the element is returned.  On failure, NULL is returned.
QueueElement *find_from_queue_by_condition(Queue *queue, FuncMatch fp_match, void *data);

// Description: Add a process to the suspend queue.
// Parameter @pcb: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.
int add_to_suspend_queue(PCB *pcb);

// Description: Check if there are same names of processes.
// Parameter @name: The name to be checked.
// Return: 0 indicates the name conflicts with the other name of a process.
// 1 indicates the name is a valid one.
int validate_duplicate_process_name(const char *name);

// Description: validate the length of the name.
// Parameter @name: The name to be checked.
// Return: 1 indicates the name is valid. 0 indicates the name is too long.
int check_length_of_process_name(const char *name);

// Description: validate the priority.
// Parameter @priority: The priority to be checked.
// Return: 1 indicates the priority is valid.
// 0 indicates the priority is too low or too high.
int validate_priority_range(INT32 priority);

// Description: Generate the process id.
// Return: On success, the ID of the process is returned.
// On error, -1 is returned indicates the number of processes
// has reached the maximum number of processes.
INT32 pid_generator(void);

// Description: Create a PCB. On successful creation,
// the PCB will be added to the ready queue.
// Parameter @name: The name of the PCB.
// Parameter @start_point: The starting point of the PCB.
// Parameter @priority: The priority of the PCB.
// Parameter @error: The error returned from the function.
// Return: On success, a pointer of the PCB is returned.  On error, NULL is returned.
PCB *create_pcb(const char *name, void *start_point, INT32 priority,
        void *context, long *error);

// Description: Create a process.
// Parameter @name: The name of the process.
// Parameter @start_point: The starting point of the process.
// Parameter @priority: The priority of the process.
// Parameter @error: The error returned from the function.
// Return: On success, a pointer of the process is returned.  On error, NULL is returned.
PCB *os_create_process(const char *name, void *start_point, INT32 priority,
        long *error);

// Description: Make a process sleep for a certain mount of time.
// Add the process who needs to sleep to the timer queue, and start the timer.
// Parameter @sleep_time: The time within which the process is going to sleep.
// Return: None.
void os_process_sleep(long sleep_time);

// Description: Make the timed up processes ready to run. (Called in the interrupt handler).
// Dequeue the timed up processes from the timer queue and put them into the ready queue
// or the suspend queue, then reset the timer.
// Parameter: None.
// Return: None.
void os_make_ready_to_run(void);

// Description: Dispatch a process to make it the running one.
// Dequeue the first item from the ready queue and
// make it the running process by context switching.
// or the suspend queue, then reset the timer.
// Parameter: None.
// Return: None.
void os_dispatcher(void);

// Description: Terminate certain process. If the pid equals -1 or the ID
// of the running process, then remove it from the global process table.
// Dequeue the first item in the ready queue, and switch to it. Or delete
// the process with the ID from every position where it exists.
// Parameter @pid: The process to be terminated.
// Parameter @name: The error returned from the function.
// Return: None.
void os_terminate_process(INT32 pid, long *error);

// Description: Get the ID of a process. If the name is "" or
// the same as the name of current process, then return the ID
// of current id. Otherwise, return the name of certain process.
// Parameter @name: The name of a process to be found.
// Parameter @pid: The ID of the found process, if not found, pid equals -1.
// Parameter @error: The error returned from the function.
// Return: None.
void os_get_process_id(const char *name, INT32 *pid, long *error);

// Description: Suspend a process with given PID. Look up in timer queue
// and ready queue to find the certain process. If in timer queue, just
// set the suspend field of that process to be TRUE. If in ready queue,
// put it to the suspend queue and set the suspend field.
// Parameter @pid: The ID of the process to be suspended
// Parameter @error: The error returned from the function.
// Return: None.
void os_suspend_process(INT32 pid, long *error);

// Description: Resume a process with given PID. Look up in timer queue
// and ready queue to find the certain process. If in timer queue, just
// set the suspend field of that process to be FALSE. If in ready queue,
// put it to the suspend queue and set the suspend field.
// Parameter @pid: The ID of the process to be resumed
// Parameter @error: The error returned from the function.
// Return: None.
void os_resume_process(INT32 pid, long *error);

// Description: Change the priority of a process. After change
// the priority of a process, when the process is added to the
// ready queue, the queue will be sorted by priority.
// Parameter @pid: The ID of the process to be resumed
// Parameter @error: The error returned from the function.
// Return: None.
void os_change_priority(INT32 pid, INT32 priority, long *error);

/**
 * Add a process into suspend queue, make it the first item of the queue. Used in interrupt_handler.
 * @param pcb: The process will be added.
 * @return: Indicate whether the add action succeeds.
 */
int enqueue_suspend_queue_reversly(PCB *pcb);
/**
 * Remove the process from suspend queue by given disk id.
 * @param disk_id: The disk id needs to be matched of the process.
 * @return If succeeds, the pcb is returned.
 * If the queue is empty or the operation fails, NULL is returned.
 */
PCB *remove_from_suspend_queue_by_disk_id(INT16 disk_id);

#endif	/* PROC_MGMT_H */

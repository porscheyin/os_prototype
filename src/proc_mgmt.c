#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "common.h"
#include "global.h"
#include "protos.h"
#include "os_utils.h"
#include "syscalls.h"
#include "proc_mgmt.h"
#include "data_struct.h"

PCB *RootPCB; // Indicate Initial Process.
PCB *CurrentPCB; // Indicate current Process.
Queue *TimerQueue; // Indicate the queue which contains sleeping processes.
Queue *ReadyQueue; // Indicate the queue which contains processes who are ready to be run.
Queue *SuspendQueue; // Indicate the queue which contains suspended processes.

extern Queue *DiskQueue;
extern Queue *FrameQueue;

/***For type safe, always use pointer to function as the callback argument!***/
FuncMatch fp_match; // Used as callback when find matching items.
FuncCompare fp_compare; // Used as callback when compare between items.
extern ConfigArgEntry *ConfigArgument;
PCB *ProcessTable[MAX_NUMBER_OF_USER_PROCESSES]; // Global process table, used to manage processes.

// Description: Check if the two processes match.
// Parameter @data1 & @data2: Two processes are to be checked.
// Return: 1 indicates matching.  0 indicates does not match.

int match_pcb(const void *data1, const void *data2)
{
    assert(data1 && data2);
    return (const PCB *) data1 == (const PCB *) data2;
}

// Description: Check if the two process IDs match.
// Parameter @data1: The process whose ID to be compared.
// Parameter @pid: The processes ID to be compared.
// Return: 1 indicates matching.  0 indicates does not match.

int match_pid(const void *data1, const void *pid)
{
    assert(data1 && pid);
    return ((const PCB *) data1)->pid == (INT32) pid;
}

int match_disk_id(const void *data1, const void *disk_id)
{
    assert(data1 && disk_id);
    return ((const PCB *) data1)->disk_id == (INT16) disk_id;
}


// Description: Compare the delay time of two processes.
// Parameter @data1 & @data2: Two processes whose delay time are to be compared.
// Return: return an integer less than, equal to, or greater than zero
// if the delay time of data1 is found, respectively, to be less than,
// to match, or be greater than that of data2.

int compare_time(const void *data1, const void *data2)
{
    assert(data1 && data2);
    if (((const PCB *) data1)->delay_time > ((const PCB *) data2)->delay_time)
    {
        return 1;
    }
    else if (((const PCB *) data1)->delay_time
            == ((const PCB *) data2)->delay_time)
        return 0;
    else
        return -1;
}

// Description: Compare the priority of two processes.
// Parameter @data1 & @data2: Two processes whose priority are to be compared.
// Return: return an integer less than, equal to, or greater than zero
// if the priority of data1 is found, respectively, to be less than,
// to match, or be greater than that of data2.

int compare_priority(const void *data1, const void *data2)
{
    if (((const PCB *) data1)->priority > ((const PCB *) data2)->priority)
        return 1;
    else if (((const PCB *) data1)->priority == ((const PCB *) data2)->priority)
        return 0;
    else
        return -1;
}

// Description: Initialize the global process table.
// Parameter: None.
// Return: None.

void init_process_table()
{
    int i;
    for (i = 0; i < MAX_NUMBER_OF_USER_PROCESSES; i++)
        ProcessTable[i] = NULL;
}

// Description: Create and initialize several queues.
// Parameter: None.
// Return: On success, 1 is returned.  On error, 0 is returned.

int init_queues()
{
    if ((TimerQueue = queue_create()) == NULL)
        return 0;
    if ((ReadyQueue = queue_create()) == NULL)
        return 0;
    if ((SuspendQueue = queue_create()) == NULL)
        return 0;
    if ((DiskQueue = queue_create()) == NULL)
        return 0;
    if ((FrameQueue = queue_create()) == NULL)
        return 0;
    return 1;
}

// Description: Add a process to the global process table.
// Parameter @pcb_to_add: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.

int add_to_process_table(PCB *pcb_to_add)
{
    if (pcb_to_add)
    {
        ProcessTable[pcb_to_add->pid] = pcb_to_add;
        return 1;
    }

    else
        return 0;
}

// Description: Remove a process from the global process table.
// Parameter @pcb_to_remove: The process to remove.
// Return: On success, 1 is returned.  On error, 0 is returned.

int remove_from_process_table(PCB *pcb_to_remove)
{
    if (pcb_to_remove)
    {
        if (ProcessTable[pcb_to_remove->pid] != NULL)
        {
            ProcessTable[pcb_to_remove->pid] = NULL;
            free(pcb_to_remove);
            return 1;
        }
        else
            return 0;
    }
    else
        return 0;
}

// Description: Print scheduling information by using scheduler printer.
// Parameter @action_mode: The string indicates the action mode.
// Parameter @target_pcb: The process on which the scheduler action is being performed.
// Parameter @info_type: indicate if print the final end information.
// Return: None.

void print_scheduling_info(char *action_mode, PCB *target_pcb, int info_type)
{
    QueueElement *element;
    static INT32 how_many_interrupt_entries = 0;

    if (ConfigArgument->show_scheduler_output == Full)
    {
        // Set action mode, target pid and running pid.
        CALL(SP_setup_action(SP_ACTION_MODE, action_mode));
        CALL(SP_setup(SP_TARGET_MODE, target_pcb->pid));
        CALL(SP_setup(SP_RUNNING_MODE, CurrentPCB->pid));
        if (strcmp(action_mode, ACTION_NAME_CREATE) == 0)
            CALL(SP_setup(SP_NEW_MODE, target_pcb->pid));
        if (strcmp(action_mode, ACTION_NAME_DONE) == 0)
            CALL(SP_setup(SP_TERMINATED_MODE, target_pcb->pid));

        if (info_type == NORMAL_INFO)
        {
            // Attach all pid in all queues to the printer.
            if (!queue_is_empty(TimerQueue))
            {
                for (element = queue_head(TimerQueue); element; element =
                        queue_next(element))
                {
                    CALL(
                         SP_setup(SP_WAITING_MODE, ((PCB *) element->data)->pid));
                }
            }
            if (!queue_is_empty(ReadyQueue))
            {
                for (element = queue_head(ReadyQueue); element; element =
                        queue_next(element))
                {
                    CALL(SP_setup(SP_READY_MODE, ((PCB *) element->data)->pid));
                }
            }
            if (!queue_is_empty(SuspendQueue))
            {
                for (element = queue_head(SuspendQueue); element; element =
                        queue_next(element))
                {
                    CALL(
                         SP_setup(SP_SUSPENDED_MODE, ((PCB *) element->data)->pid));
                }
            }
        }
        else if (info_type == FINAL_INFO)
        {
            if (!queue_is_empty(TimerQueue))
            {
                for (element = queue_head(TimerQueue); element; element =
                        queue_next(element))
                {
                    CALL(
                         SP_setup(SP_TERMINATED_MODE, ((PCB *) element->data)->pid));
                }
            }

            if (!queue_is_empty(ReadyQueue))
            {
                for (element = queue_head(ReadyQueue); element; element =
                        queue_next(element))
                {
                    CALL(
                         SP_setup(SP_TERMINATED_MODE, ((PCB *) element->data)->pid));
                }
            }
            if (!queue_is_empty(SuspendQueue))
            {
                for (element = queue_head(SuspendQueue); element; element =
                        queue_next(element))
                {
                    CALL(
                         SP_setup(SP_TERMINATED_MODE, ((PCB *) element->data)->pid));
                }
            }
        }
        // For clarity, show the header every 10 lines.
        //if ((line_count++) % 10 == 0)
        //{
        CALL(SP_print_header());
        //}
        // Dump all contents.
        CALL(SP_print_line());
    }
    else if (ConfigArgument->show_scheduler_output == Limited)
    {
        how_many_interrupt_entries++;
        if (how_many_interrupt_entries < 10)
        {
            // Set action mode, target pid and running pid.
            CALL(SP_setup_action(SP_ACTION_MODE, action_mode));
            CALL(SP_setup(SP_TARGET_MODE, target_pcb->pid));
            CALL(SP_setup(SP_RUNNING_MODE, CurrentPCB->pid));
            if (strcmp(action_mode, ACTION_NAME_CREATE) == 0)
                CALL(SP_setup(SP_NEW_MODE, target_pcb->pid));
            if (strcmp(action_mode, ACTION_NAME_DONE) == 0)
                CALL(SP_setup(SP_TERMINATED_MODE, target_pcb->pid));

            if (info_type == NORMAL_INFO)
            {
                // Attach all pid in all queues to the printer.
                if (!queue_is_empty(TimerQueue))
                {
                    for (element = queue_head(TimerQueue); element; element =
                            queue_next(element))
                    {
                        CALL(
                             SP_setup(SP_WAITING_MODE, ((PCB *) element->data)->pid));
                    }
                }
                if (!queue_is_empty(ReadyQueue))
                {
                    for (element = queue_head(ReadyQueue); element; element =
                            queue_next(element))
                    {
                        CALL(SP_setup(SP_READY_MODE, ((PCB *) element->data)->pid));
                    }
                }
                if (!queue_is_empty(SuspendQueue))
                {
                    for (element = queue_head(SuspendQueue); element; element =
                            queue_next(element))
                    {
                        CALL(
                             SP_setup(SP_SUSPENDED_MODE, ((PCB *) element->data)->pid));
                    }
                }
            }
            else if (info_type == FINAL_INFO)
            {
                if (!queue_is_empty(TimerQueue))
                {
                    for (element = queue_head(TimerQueue); element; element =
                            queue_next(element))
                    {
                        CALL(
                             SP_setup(SP_TERMINATED_MODE, ((PCB *) element->data)->pid));
                    }
                }

                if (!queue_is_empty(ReadyQueue))
                {
                    for (element = queue_head(ReadyQueue); element; element =
                            queue_next(element))
                    {
                        CALL(
                             SP_setup(SP_TERMINATED_MODE, ((PCB *) element->data)->pid));
                    }
                }
                if (!queue_is_empty(SuspendQueue))
                {
                    for (element = queue_head(SuspendQueue); element; element =
                            queue_next(element))
                    {
                        CALL(
                             SP_setup(SP_TERMINATED_MODE, ((PCB *) element->data)->pid));
                    }
                }
            }
            // For clarity, show the header every 10 lines.
            //if ((line_count++) % 10 == 0)
            //{
            CALL(SP_print_header());
            //}
            // Dump all contents.
            CALL(SP_print_line());
        }
    }
}

// Used for debugging, print information of all processes in the process table.

void print_process_table(void)
{
    int i;

    printf("\nPID     NAME                PRIORITY        DELAY       ENTRY       \n");

    for (i = 0; i < MAX_NUMBER_OF_USER_PROCESSES; i++)
    {
        if (ProcessTable[i])
        {

            printf("%-8d%-20s%-16d%-12d%-12p\n", ProcessTable[i]->pid,
                   ProcessTable[i]->process_name, ProcessTable[i]->priority,
                   ProcessTable[i]->delay_time, ProcessTable[i]->entry_point);
        }
    }
    printf("\n");
}

// Used for debugging, print information of the given process.

void print_pcb(PCB *pcb)
{
    if (pcb)
    {
        printf(
               "\nPID     NAME                PRIORITY        DELAY       ENTRY       \n");
        printf("%-8d%-20s%-16d%-12d%-12p\n", pcb->pid, pcb->process_name,
               pcb->priority, pcb->delay_time, pcb->entry_point);
        printf("\n");
    }
    else
    {

        printf("No process is running now!\n");
    }
}

// Used for debugging, print information of all processes in the given queue.

void print_queue(Queue *queue)
{
    PCB *pcb;
    QueueElement *element;

    assert(queue);

    if (queue_is_empty(queue))
    {
        printf("Queue is empty!\n");
    }
    else
    {
        printf(
               "\nPID     NAME                PRIORITY        DELAY       ENTRY       \n");

        for (element = queue_head(queue); element;
                element = queue_next(element))
        {

            pcb = (PCB *) queue_data(element);
            printf("%-8d%-20s%-16d%-12d%-12p\n", pcb->pid, pcb->process_name,
                   pcb->priority, pcb->delay_time, pcb->entry_point);
        }
    }
    printf("\n");
}

// Description: Add a process to the ready queue.
// Parameter @pcb: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.

int add_to_ready_queue(PCB *pcb)
{
    if (pcb)
    {
        fp_match = match_pcb;
        fp_compare = compare_priority;
        if (!queue_contains(ReadyQueue, fp_match, pcb))
        {
            fp_compare = compare_priority;
            if (queue_enqueue_orderly(ReadyQueue, fp_compare, ASCENDING, pcb))
                return 1;
            return 0;
        }
        return 1;
    }
    else
        return 0;
}

// Description: Remove a process from the ready queue.
// Parameter @pcb: The process returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int remove_from_ready_queue(PCB **pcb)
{
    QueueElement *element;

    if (pcb && *pcb)
    {
        fp_match = match_pcb;
        if ((element = queue_find_element(ReadyQueue, fp_match, *pcb)) != NULL)
        {
            if (queue_remove_element(ReadyQueue, element, (void **) pcb))
                return 1;
            return 0;
        }

        else
            return 0;
    }
    return 0;
}

// Description: Dequeue a process from the ready queue at the head.
// Parameter @pcb: The process returned from the dequeued element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int dequeue_from_ready_queue(PCB **pcb)
{
    if (pcb)
    {
        if (queue_dequeue(ReadyQueue, (void **) pcb))
            return 1;
        return 0;
    }

    else
        return 0;
}

// Description: Add a process to the timer queue.
// Parameter @pcb: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.

int add_to_timer_queue(PCB *pcb)
{
    if (pcb)
    {
        fp_match = match_pcb;
        if (!queue_contains(TimerQueue, fp_match, pcb))
        {
            fp_compare = compare_time;
            if (queue_enqueue_orderly(TimerQueue, fp_compare, ASCENDING, pcb))
                return 1;
            return 0;
        }
        else
            return 1;
    }

    else
        return 0;
}

// Description: Remove a process from the timer queue.
// Parameter @pcb: The process returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int remove_from_timer_queue(PCB **pcb)
{
    QueueElement *element;

    if (pcb && *pcb)
    {
        fp_match = match_pcb;
        if ((element = queue_find_element(TimerQueue, fp_match, *pcb)) != NULL)
        {
            if (queue_remove_element(TimerQueue, element, (void **) pcb))
                return 1;
            return 0;
        }

        else
            return 0;
    }
    return 0;
}

// Description: Dequeue a process from the timer queue at the head.
// Parameter @pcb: The process returned from the dequeued element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int dequeue_from_timer_queue(PCB **pcb)
{
    if (pcb)
    {
        if (queue_dequeue(TimerQueue, (void **) pcb))
            return 1;
        return 0;
    }

    else
        return 0;
}

// Description: Find an element in a queue according to specific condition.
// Parameter @queue: The queue where the pid will be found.
// Parameter @fp_match: The callback used to match data in the element.
// Parameter @data: The condition must be matched.
// Return: On success, the pointer of the element is returned.  On failure, NULL is returned.

QueueElement *find_from_queue_by_condition(Queue *queue, FuncMatch fp_match, void *data)
{
    assert(queue && fp_match);
    return queue_find_element(queue, fp_match, data);
}

// Description: Add a process to the suspend queue.
// Parameter @pcb: The process to add.
// Return: On success, 1 is returned.  On error, 0 is returned.

int add_to_suspend_queue(PCB *pcb)
{
    if (pcb)
    {
        fp_match = match_pcb;
        if (!queue_contains(SuspendQueue, fp_match, pcb))
        {
            if (queue_enqueue(SuspendQueue, pcb))
                return 1;
            return 0;
        }
        else
            return 1;
    }
    else
        return 0;
}

// Description: Check if there are same names of processes.
// Parameter @name: The name to be checked.
// Return: 0 indicates the name conflicts with the other name of a process.
// 1 indicates the name is a valid one.

int validate_duplicate_process_name(const char *name)
{
    int i;

    for (i = 0; i < MAX_NUMBER_OF_USER_PROCESSES; i++)
    {
        if (ProcessTable[i] != NULL)
        {
            if (strcmp(ProcessTable[i]->process_name, name) == 0)
            {
                return 0;
            }
        }
    }
    return 1;
}

// Description: validate the length of the name.
// Parameter @name: The name to be checked.
// Return: 1 indicates the name is valid. 0 indicates the name is too long.

int check_length_of_process_name(const char *name)
{
    return strlen(name) <= MAX_NUMBER_OF_PROCESSE_NAME;
}

// Description: validate the priority.
// Parameter @priority: The priority to be checked.
// Return: 1 indicates the priority is valid.
// 0 indicates the priority is too low or too high.

int validate_priority_range(INT32 priority)
{
    if (priority < 0 || priority > 100)
        return 0;

    else
        return 1;
}

// Description: Generate the process id.
// Return: On success, the ID of the process is returned.
// On error, -1 is returned indicates the number of processes
// has reached the maximum number of processes.

INT32 pid_generator(void)
{
    int i;
    INT32 pid = -1;
    for (i = 0; i < MAX_NUMBER_OF_USER_PROCESSES; i++)
    {
        if (ProcessTable[i] == NULL)
        {
            return pid = i;
        }
    }
    return pid;
}

// Description: Create a PCB. On successful creation,
// the PCB will be added to the ready queue.
// Parameter @name: The name of the PCB.
// Parameter @start_point: The starting point of the PCB.
// Parameter @priority: The priority of the PCB.
// Parameter @error: The error returned from the function.
// Return: On success, a pointer of the PCB is returned.  On error, NULL is returned.

PCB *create_pcb(const char *name, void *start_point, INT32 priority,
                void *context, long *error)
{
    PCB *pcb;
    INT32 pid;
    int result;

    assert(name && start_point && context && error);

    result = check_length_of_process_name(name);

    do
    {
        if (!result)
        {
            *error = ERR_EXCEED_MAX_PROCESS_NAME_NUMBER;
            pcb = NULL;
            break;
        }
        result = validate_duplicate_process_name(name);
        if (!result)
        {
            *error = ERR_DUPLICATE_PROCESS_NAME;
            pcb = NULL;
            break;
        }
        result = validate_priority_range(priority);
        if (!result)
        {
            *error = ERR_ILLEGAL_PRIORITY;
            pcb = NULL;
            break;
        }
        pid = pid_generator();
        if (pid < 0)
        {
            *error = ERR_EXCEED_MAX_PROCESS_NUMBER;
            pcb = NULL;
            break;
        }
        pcb = malloc(sizeof *pcb);
        if (!pcb)
        {
            *error = ERR_OS502_GENERATED_BUG;
            printf("Create_pcb: malloc fails!\n");
            shut_down();
        }
        pcb->pid = pid;
        pcb->context = context;
        pcb->priority = priority;
        pcb->suspend = FALSE;
        pcb->need_message = FALSE;
        pcb->entry_point = start_point;
        pcb->disk_id = pid / 2 + 1;
        pcb->operation = -1;
        strncpy(pcb->process_name, name, strlen(name) + 1);

        result = add_to_process_table(pcb);
        if (!result)
        {
            error_message("add_to_process_table");
            shut_down();
        }
        *error = ERR_SUCCESS;
    }
    while (0);

    return pcb;
}

// Description: Create a process.
// Parameter @name: The name of the process.
// Parameter @start_point: The starting point of the process.
// Parameter @priority: The priority of the process.
// Parameter @error: The error returned from the function.
// Return: On success, a pointer of the process is returned.  On error, NULL is returned.

PCB *os_create_process(const char *name, void *start_point, INT32 priority,
                       long *error)
{
    PCB *pcb;
    int result;
    void *next_context;

    do
    {
        // Validate parameters.
        if (error == NULL)
        {
            printf("Argument is NULL: os_create_process!\n");
            pcb = NULL;
            break;
        }
        if (name == NULL || start_point == NULL)
        {
            *error = ERR_BAD_PARAM;
            printf("Argument is NULL: os_create_process!\n");
            pcb = NULL;
            break;
        }

        make_context(&next_context, start_point, USER_MODE);
        if (!next_context)
        {
            *error = ERR_Z502_INTERNAL_BUG;
            error_message("Creation of context fails!\n");
            shut_down();
        }

        get_data_lock(COMMON_DATA_LOCK);
        get_data_lock(TIMER_QUEUE_LOCK);
        get_data_lock(READY_QUEUE_LOCK);

#ifdef DEBUG_PROCESS
        print_process_table();
#endif

        CALL(pcb = create_pcb(name, start_point, priority, next_context, error));

        if (pcb)
        {
            //After creation, the process is added to ReadyQueue.
            result = add_to_ready_queue(pcb);
            if (!result)
            {
                error_message("add_to_ready_queue");
                shut_down();
            }
            break;
        }
    }
    while (0);
    release_data_lock(READY_QUEUE_LOCK);
    release_data_lock(TIMER_QUEUE_LOCK);
    release_data_lock(COMMON_DATA_LOCK);
    return pcb;
}

// Description: Make a process sleep for a certain mount of time.
// Add the process who needs to sleep to the timer queue, and start the timer.
// Parameter @sleep_time: The time within which the process is going to sleep.
// Return: None.

void os_process_sleep(long sleep_time)
{
    int result;
    INT32 status;
    INT32 time_now;

    get_data_lock(COMMON_DATA_LOCK);

#ifdef DEBUG_STAGE
    stage_info(CurrentPCB, "Lock owned!");
#endif

    // If sleep time smaller than 0, just return.
    if (sleep_time < 0)
    {
        printf("Sleep time should not be less than 0!\n");
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if (sleep_time == 0)
    {
        // If sleep time equals 0, add current PCB into ReadyQueue.
        get_data_lock(TIMER_QUEUE_LOCK);
        get_data_lock(READY_QUEUE_LOCK);
        CALL(result = add_to_ready_queue(CurrentPCB));
        if (result)
            print_scheduling_info(ACTION_NAME_READY, CurrentPCB, NORMAL_INFO);
        if (!result)
        {
            error_message("add_to_ready_queue");
            shut_down();
        }
        release_data_lock(READY_QUEUE_LOCK);
        release_data_lock(TIMER_QUEUE_LOCK);
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else
    {
        // Set delay time and add current PCB into TimerQueue.
        time_now = get_current_time();
        CurrentPCB->delay_time = time_now + sleep_time;

        get_data_lock(TIMER_QUEUE_LOCK);
        get_data_lock(READY_QUEUE_LOCK);
        fp_match = match_pcb;
        if (queue_contains(ReadyQueue, fp_match, CurrentPCB))
        {
            CALL(result = remove_from_ready_queue(&CurrentPCB));
        }
        if (!result)
        {
            error_message("remove_from_ready_queue");
            shut_down();
        }

        CALL(result = add_to_timer_queue(CurrentPCB));

        if (result)
            print_scheduling_info(ACTION_NAME_WAIT, CurrentPCB, NORMAL_INFO);
        else
        {
            error_message("add_to_timer_queue");
            shut_down();
        }

        // Start timer.
        status = get_timer_status();

        int time = ((PCB *) queue_head(TimerQueue)->data)->delay_time;

        if (status == DEVICE_FREE
                || (status == DEVICE_IN_USE && CurrentPCB->delay_time < time))
        {
            start_timer((INT32 *) & sleep_time);
        }
        release_data_lock(READY_QUEUE_LOCK);
        release_data_lock(TIMER_QUEUE_LOCK);
    }

    release_data_lock(COMMON_DATA_LOCK);

#ifdef DEBUG_STAGE
    stage_info(CurrentPCB, "Lock released!");
#endif
    return;
}

// Description: Make the timed up processes ready to run. (Called in the interrupt handler).
// Dequeue the timed up processes from the timer queue and put them into the ready queue
// or the suspend queue, then reset the timer.
// Parameter: None.
// Return: None.

void os_make_ready_to_run(void)
{
    PCB *pcb;
    int result;
    INT32 time_now;
    long time_to_sleep;

#ifdef DEBUG_STAGE
    stage_info(CurrentPCB, "Enter make_ready_to_run...");
#endif

    get_data_lock(COMMON_DATA_LOCK);
    get_data_lock(TIMER_QUEUE_LOCK);
    get_data_lock(READY_QUEUE_LOCK);

    // Dequeue first element from TimerQueue.
    if (!queue_is_empty(TimerQueue))
    {
        result = dequeue_from_timer_queue(&pcb);

        if (!result)
        {
            error_message("dequeue_from_timer_queue");
            shut_down();
        }
        pcb->delay_time = 0;

        // If the PCB is not supposed to be suspended, add it to the ReadyQueue.
        if (pcb->suspend == FALSE)
        {
            result = add_to_ready_queue(pcb);
            if (result)
                print_scheduling_info(ACTION_NAME_READY, pcb, NORMAL_INFO);
            else
            {
                error_message("add_to_ready_queue");
                shut_down();
            }
        }
        else
        {
            // Or add it to the SuspendQueue.
            get_data_lock(SUSPEND_QUEUE_LOCK);
            result = add_to_suspend_queue(pcb);
            release_data_lock(SUSPEND_QUEUE_LOCK);
            if (result)
                print_scheduling_info(ACTION_NAME_SUSPEND, pcb, NORMAL_INFO);
            else
            {
                error_message("add_to_suspend_queue");
                shut_down();
            }
        }

#ifdef DEBUG_STAGE
        if (result)
            stage_info(CurrentPCB, "Added to ready queue.\n");
#endif

        // If there are more PCBs whose delay time is smaller than now,
        // add them to the appropriate queue.
        time_now = get_current_time();
        if (!queue_is_empty(TimerQueue))
        {
            pcb = (PCB *) queue_data(queue_head(TimerQueue));
        }
        while (!queue_is_empty(TimerQueue) && pcb->delay_time <= time_now)
        {
            result = dequeue_from_timer_queue(&pcb);
            if (!result)
            {
                error_message("dequeue_from_timer_queue");
                shut_down();
            }
            pcb->delay_time = 0;

            if (pcb->suspend == FALSE)
            {
                result = add_to_ready_queue(pcb);
                if (result)
                    print_scheduling_info(ACTION_NAME_READY, pcb, NORMAL_INFO);
                else
                {
                    error_message("add_to_ready_queue");
                    shut_down();
                }
            }
            else
            {
                get_data_lock(SUSPEND_QUEUE_LOCK);
                result = add_to_suspend_queue(pcb);
                release_data_lock(SUSPEND_QUEUE_LOCK);
                if (result)
                    print_scheduling_info(ACTION_NAME_SUSPEND, pcb, NORMAL_INFO);
                else
                {
                    error_message("add_to_suspend_queue");
                    shut_down();
                }
            }
            if (!queue_is_empty(TimerQueue))
            {
                time_now = get_current_time();
                pcb = (PCB *) queue_data(queue_head(TimerQueue));
            };
        }

        // Reset the timer.
        if (!queue_is_empty(TimerQueue))
        {
            time_now = get_current_time();
            pcb = (PCB *) queue_data(queue_head(TimerQueue));
            time_to_sleep = pcb->delay_time - time_now;
            start_timer((INT32 *) & time_to_sleep);
        }
    }
    release_data_lock(READY_QUEUE_LOCK);
    release_data_lock(TIMER_QUEUE_LOCK);
    release_data_lock(COMMON_DATA_LOCK);

    return;
}

// Description: Dispatch a process to make it the running one.
// Dequeue the first item from the ready queue and
// make it the running process by context switching.
// or the suspend queue, then reset the timer.
// Parameter: None.
// Return: None.

void os_dispatcher(void)
{
    int result;

#ifdef DEBUG_STAGE
    stage_info(CurrentPCB, "Enter dispather...");
#endif

    // Wait until ReadyQueue is not null.
    while (queue_is_empty(ReadyQueue))
    {
        idle_and_wait();

#ifdef DEBUG_STAGE
        stage_info(CurrentPCB, "Wait: Nothing in ReadyQueue!");
#endif
    }

    get_data_lock(COMMON_DATA_LOCK);
    get_data_lock(TIMER_QUEUE_LOCK);
    get_data_lock(READY_QUEUE_LOCK);
    CALL(result = dequeue_from_ready_queue(&CurrentPCB));
    if (result)
        print_scheduling_info(ACTION_NAME_DISPATCH, CurrentPCB, NORMAL_INFO);
    else
    {

        error_message("dequeue_from_ready_queue");
        shut_down();
    }

#ifdef DEBUG_STAGE
    stage_info(CurrentPCB, "Before context switch.");
#endif

    release_data_lock(READY_QUEUE_LOCK);
    release_data_lock(TIMER_QUEUE_LOCK);
    release_data_lock(COMMON_DATA_LOCK);

    switch_context(SWITCH_CONTEXT_SAVE_MODE, &(CurrentPCB->context));
}

// Description: Terminate certain process. If the pid equals -1 or the ID
// of the running process, then remove it from the global process table.
// Dequeue the first item in the ready queue, and switch to it. Or delete
// the process with the ID from every position where it exists.
// Parameter @pid: The process to be terminated.
// Parameter @name: The error returned from the function.
// Return: None.

void os_terminate_process(INT32 pid, long *error)
{
    int result;

    assert(error);

    get_data_lock(COMMON_DATA_LOCK);

    // Validate parameters.
    if (pid < -2 || pid >= MAX_NUMBER_OF_USER_PROCESSES)
    {
        *error = ERR_BAD_PARAM;
        return;
    }
    else if (pid == -1 || CurrentPCB->pid == pid) // Terminate current process.
    {
        if (CurrentPCB->pid == RootPCB->pid)
        {
            print_scheduling_info(ACTION_NAME_ALLDONE, CurrentPCB, FINAL_INFO);
            release_data_lock(COMMON_DATA_LOCK);
            shut_down();
        }
        else
        {
            release_data_lock(COMMON_DATA_LOCK);

            while (queue_is_empty(ReadyQueue))
            {
                // If no process is ready, just wait.
                idle_and_wait();
            }

#ifdef DEBUG_PROCESS
            print_process_table();
#endif

            get_data_lock(COMMON_DATA_LOCK);

            // Remove current pcb from global process table.
            result = remove_from_process_table(CurrentPCB);

#ifdef DEBUG_PROCESS
            if (result)
                print_process_table();
#endif
            if (!result)
            {
                error_message("remove_from_process_table");
                shut_down();
            }

            get_data_lock(TIMER_QUEUE_LOCK);
            get_data_lock(READY_QUEUE_LOCK);

            CALL(result = dequeue_from_ready_queue(&CurrentPCB));
            if (result)
                print_scheduling_info(ACTION_NAME_DONE, CurrentPCB, NORMAL_INFO);
            else
            {
                error_message("dequeue_from_ready_queue");
                shut_down();
            }
            *error = ERR_SUCCESS;
            release_data_lock(READY_QUEUE_LOCK);
            release_data_lock(TIMER_QUEUE_LOCK);
            release_data_lock(COMMON_DATA_LOCK);
#ifdef DEBUG_STAGE
            stage_info(CurrentPCB, "Lock released!");
            stage_info(CurrentPCB, "Before Context switch!");
#endif
            switch_context(SWITCH_CONTEXT_SAVE_MODE, &(CurrentPCB->context));
        }
    }
    else if (pid == -2) // Terminate all processes, end the simulation.
    {
        print_scheduling_info(ACTION_NAME_ALLDONE, CurrentPCB, FINAL_INFO);
        shut_down();
    }
    else // Delete the process with the pid from every position where it exists.
    {
        PCB *pcb;
        size_t size;
        INT32 time_now;
        long time_to_sleep;
        QueueElement *element;
        fp_match = match_pid;

        get_data_lock(TIMER_QUEUE_LOCK);
        get_data_lock(READY_QUEUE_LOCK);

        // Find from TimerQueue.
        element = find_from_queue_by_condition(TimerQueue, fp_match, (void *) pid);
        if (element)
        {
            if ((size = queue_size(TimerQueue)) == 1)
            {
                // Close timer.
                time_to_sleep = 0;
                start_timer((INT32 *) & time_to_sleep);
            }
            else if (size > 1 && queue_is_head(TimerQueue, element))
            {
                time_now = get_current_time();
                pcb = (PCB *) queue_data(queue_next(element));
                time_to_sleep = pcb->delay_time - time_now;
                start_timer((INT32 *) & time_to_sleep);
            }
            result = queue_remove_element(TimerQueue, element, (void **) &pcb);
            if (!result)
            {
                error_message("queue_remove_element");
                shut_down();
            }
        }
        // Find from ReadyQueue.
        element = find_from_queue_by_condition(ReadyQueue, fp_match, (void *) pid);
        if (element != NULL)
        {
            result = queue_remove_element(ReadyQueue, element, (void **) &pcb);
            if (result)
                print_scheduling_info(ACTION_NAME_DONE, CurrentPCB, NORMAL_INFO);
            else
            {
                error_message("queue_remove_element");
                shut_down();
            }
        }
        // Find from SuspendQueue.
        element = find_from_queue_by_condition(SuspendQueue, fp_match, (void *) pid);
        if (element != NULL)
        {
            result = queue_remove_element(SuspendQueue, element,
                                          (void **) &pcb);
            if (result)
                print_scheduling_info(ACTION_NAME_DONE, CurrentPCB, NORMAL_INFO);
            else
            {
                error_message("queue_remove_element");
                shut_down();
            }
        }
#ifdef DEBUG_PROCESS
        print_process_table();
#endif

        // Remove from global process table.
        result = remove_from_process_table(pcb);

#ifdef DEBUG_PROCESS
        if (result)
            print_process_table();
#endif
        if (!result)
        {
            error_message("remove_from_process_table");
            shut_down();
        }
        else
        {
            *error = ERR_SUCCESS;
        }
        release_data_lock(READY_QUEUE_LOCK);
        release_data_lock(TIMER_QUEUE_LOCK);
        release_data_lock(COMMON_DATA_LOCK);
    }
    return;
}

// Description: Get the ID of a process. If the name is "" or
// the same as the name of current process, then return the ID
// of current id. Otherwise, return the name of certain process.
// Parameter @name: The name of a process to be found.
// Parameter @pid: The ID of the found process, if not found, pid equals -1.
// Parameter @error: The error returned from the function.
// Return: None.

void os_get_process_id(const char *name, INT32 *pid, long *error)
{
    PCB *pcb;
    int i, result;

    assert(name && pid && error);

    get_data_lock(COMMON_DATA_LOCK);

    // Validate parameters.
    result = check_length_of_process_name(name);
    if (!result)
    {
        *pid = -1;
        *error = ERR_EXCEED_MAX_PROCESS_NAME_NUMBER;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if (strcmp("", name) == 0
            || strcmp(CurrentPCB->process_name, name) == 0) // Get the pid of current process.
    {
        *pid = CurrentPCB->pid;
        *error = ERR_SUCCESS;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else // Get the pid of the process with certain name by looking up in the global process table.
    {
        for (i = 0; i < MAX_NUMBER_OF_USER_PROCESSES; i++)
        {
            pcb = ProcessTable[i];
            if (pcb != NULL && (strcmp(pcb->process_name, name) == 0))
            {
                *pid = pcb->pid;
                *error = ERR_SUCCESS;
                release_data_lock(COMMON_DATA_LOCK);
                return;
            }
        }
        *pid = -1;
        *error = ERR_PROCESS_DOESNT_EXIST;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
}

// Description: Suspend a process with given PID. Look up in timer queue
// and ready queue to find the certain process. If in timer queue, just
// set the suspend field of that process to be TRUE. If in ready queue,
// put it to the suspend queue and set the suspend field.
// Parameter @pid: The ID of the process to be suspended
// Parameter @error: The error returned from the function.
// Return: None.

void os_suspend_process(INT32 pid, long *error)
{
    PCB *pcb;
    int result;
    fp_match = match_pid;
    QueueElement *element;

    get_data_lock(COMMON_DATA_LOCK);
    if (pid < -1 || pid >= MAX_NUMBER_OF_USER_PROCESSES) // Validate the process id.
    {
        *error = ERR_PROCESS_DOESNT_EXIST;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if ((pid == -1) || (pid == CurrentPCB->pid)) // Suspend oneself is not allowed.
    {
        *error = ERR_SUSPEND_SELF;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if (pid != -1 && ProcessTable[pid] == NULL) // Valid pid, but process with that pid does not exist.
    {
        *error = ERR_PROCESS_DOESNT_EXIST;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if (ProcessTable[pid]) // Process exists.
    {
        get_data_lock(TIMER_QUEUE_LOCK);
        get_data_lock(READY_QUEUE_LOCK);

        if (ProcessTable[pid]->suspend == TRUE) // Already suspended.
        {
            *error = ERR_ALREADY_SUSPENDED;
            release_data_lock(READY_QUEUE_LOCK);
            release_data_lock(TIMER_QUEUE_LOCK);
            release_data_lock(COMMON_DATA_LOCK);
            return;
        }
        else if ((element = find_from_queue_by_condition(TimerQueue, fp_match, (void *) pid))
                == NULL) // Not in timer queue.
        {
            if ((element = find_from_queue_by_condition(ReadyQueue, fp_match, (void *) pid))
                    != NULL) // Exists in ready queue.
            {
                result = queue_remove_element(ReadyQueue, element, (void **) &pcb);
                if (!result)
                {
                    error_message("queue_remove_element");
                    shut_down();
                }
                get_data_lock(SUSPEND_QUEUE_LOCK);
                result = add_to_suspend_queue(pcb);
                release_data_lock(SUSPEND_QUEUE_LOCK);
                if (result)
                {
                    ProcessTable[pid]->suspend = TRUE;
                    *error = ERR_SUCCESS;
                    print_scheduling_info(ACTION_NAME_SUSPEND, pcb,
                                          NORMAL_INFO);
                }
                else
                {
                    error_message("add_to_suspend_queue");
                    shut_down();
                }
            }
            else
            {
                get_data_lock(SUSPEND_QUEUE_LOCK);
                result = add_to_suspend_queue(ProcessTable[pid]);
                release_data_lock(SUSPEND_QUEUE_LOCK);
                if (result)
                {
                    ProcessTable[pid]->suspend = TRUE;
                    *error = ERR_SUCCESS;
                    print_scheduling_info(ACTION_NAME_SUSPEND,
                                          ProcessTable[pid],
                                          NORMAL_INFO);
                    release_data_lock(READY_QUEUE_LOCK);
                    release_data_lock(TIMER_QUEUE_LOCK);
                    release_data_lock(COMMON_DATA_LOCK);
                    return;
                }
                else
                {
                    error_message("add_to_suspend_queue");
                    shut_down();
                }
            }
        }
        else
        {
            // The PCB is already in TimerQueue, just make the flag true.
            ProcessTable[pid]->suspend = TRUE;
            *error = ERR_SUCCESS;
        }
        release_data_lock(READY_QUEUE_LOCK);
        release_data_lock(TIMER_QUEUE_LOCK);
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else
    {

        error_message("Other errors\n");
    }
    release_data_lock(COMMON_DATA_LOCK);
}

// Description: Resume a process with given PID. Look up in timer queue
// and syspend queue to find the certain process. If in timer queue, just
// set the suspend field of that process to be FALSE. If in syspend queue,
// put it to the ready queue and set the suspend field.
// Parameter @pid: The ID of the process to be resumed
// Parameter @error: The error returned from the function.
// Return: None.

void os_resume_process(INT32 pid, long *error)
{
    PCB *pcb;
    int result;
    fp_match = match_pid;
    QueueElement *element;

    get_data_lock(COMMON_DATA_LOCK);

    if (pid < -1 || pid >= MAX_NUMBER_OF_USER_PROCESSES) //whether violate the process id
    {
        *error = ERR_PROCESS_DOESNT_EXIST;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if ((pid == -1) || (pid == CurrentPCB->pid)) //not permitted to suspend oneself
    {
        *error = ERR_RESUME_SELF;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if (pid != -1 && ProcessTable[pid] == NULL)
    {
        *error = ERR_PROCESS_DOESNT_EXIST;
        release_data_lock(COMMON_DATA_LOCK);
        return;
    }
    else if (ProcessTable[pid])
    {
        if (ProcessTable[pid]->suspend == TRUE) // Already suspended.
        {
            get_data_lock(TIMER_QUEUE_LOCK);
            get_data_lock(READY_QUEUE_LOCK);
            get_data_lock(SUSPEND_QUEUE_LOCK);

            if ((element = find_from_queue_by_condition(TimerQueue, fp_match, (void *) pid))
                    == NULL) // Not in TimerQueue.
            {
                if ((element = find_from_queue_by_condition(SuspendQueue, fp_match, (void *) pid)) != NULL) // Exists in SuspendQueue.
                {
                    result = queue_remove_element(SuspendQueue, element, (void **) &pcb);
                    if (!result)
                    {
                        error_message("queue_remove_element");
                        shut_down();
                        return;
                    }
                    result = add_to_ready_queue(pcb);
                    if (result)
                    {
                        ProcessTable[pid]->suspend = FALSE;
                        *error = ERR_SUCCESS;
                        print_scheduling_info(ACTION_NAME_RESUME, pcb,
                                              NORMAL_INFO);
                        release_data_lock(SUSPEND_QUEUE_LOCK);
                        release_data_lock(READY_QUEUE_LOCK);
                        release_data_lock(TIMER_QUEUE_LOCK);
                        release_data_lock(COMMON_DATA_LOCK);
                        return;
                    }
                    else
                    {
                        error_message("add_to_ready_queue.");
                        shut_down();
                    }
                }
                else
                {
                    result = add_to_ready_queue(ProcessTable[pid]);
                    if (result)
                    {
                        ProcessTable[pid]->suspend = FALSE;
                        *error = ERR_SUCCESS;
                        print_scheduling_info(ACTION_NAME_RESUME,
                                              ProcessTable[pid],
                                              NORMAL_INFO);
                        release_data_lock(SUSPEND_QUEUE_LOCK);
                        release_data_lock(READY_QUEUE_LOCK);
                        release_data_lock(TIMER_QUEUE_LOCK);
                        release_data_lock(COMMON_DATA_LOCK);
                        return;
                    }
                    else
                    {
                        error_message("add_to_ready_queue.");
                        shut_down();
                    }
                }
            }
            else
            {
                ProcessTable[pid]->suspend = FALSE;
                *error = ERR_SUCCESS;
            }
            release_data_lock(SUSPEND_QUEUE_LOCK);
            release_data_lock(READY_QUEUE_LOCK);
            release_data_lock(TIMER_QUEUE_LOCK);
            release_data_lock(COMMON_DATA_LOCK);
            return;
        }
        else // The process is not suspended.
        {
            *error = ERR_RESUME_UNSUSPENDED_PROCESS;
        }
    }
    release_data_lock(COMMON_DATA_LOCK);
}

// Description: Change the priority of a process. After change
// the priority of a process, when the process is added to the
// ready queue, the queue will be sorted by priority.
// Parameter @pid: The ID of the process to be resumed
// Parameter @error: The error returned from the function.
// Return: None.

void os_change_priority(INT32 pid, INT32 priority, long *error)
{
    get_data_lock(COMMON_DATA_LOCK);
    get_data_lock(TIMER_QUEUE_LOCK);
    get_data_lock(READY_QUEUE_LOCK);

    if (!validate_priority_range(priority)) // Validate the priority.
    {
        *error = ERR_ILLEGAL_PRIORITY;
    }
    else if (pid < -1 || pid >= MAX_NUMBER_OF_USER_PROCESSES) // Validate the process id.
    {
        *error = ERR_PROCESS_DOESNT_EXIST;
    }
    else if (pid == -1 || pid == CurrentPCB->pid) // Change the priority of current process.
    {
        printf("Current PCB %d: Priority changed from %d to %d!\n",
               CurrentPCB->pid, CurrentPCB->priority, priority);
        CurrentPCB->priority = priority;
        fp_compare = compare_priority;
        queue_sort(ReadyQueue, fp_compare, ASCENDING);
        print_scheduling_info(ACTION_NAME_READY, CurrentPCB, NORMAL_INFO);
        *error = ERR_SUCCESS;
    }
    else // Change the priority of certain process.
    {
        if (ProcessTable[pid] == NULL) // Check if the process exists.
        {
            *error = ERR_PROCESS_DOESNT_EXIST;
        }
        else
        {
            printf("PCB %d: Priority changed from %d to %d!\n", pid,
                   ProcessTable[pid]->priority, priority);
            ProcessTable[pid]->priority = priority;
            fp_compare = compare_priority;
            queue_sort(ReadyQueue, fp_compare, ASCENDING);
            print_scheduling_info(ACTION_NAME_READY, ProcessTable[pid],
                                  NORMAL_INFO);
            *error = ERR_SUCCESS;
        }
    }

    release_data_lock(COMMON_DATA_LOCK);
    release_data_lock(TIMER_QUEUE_LOCK);
    release_data_lock(READY_QUEUE_LOCK);
    return;
}

/**
 * Add a process into suspend queue, make it the first item of the queue. Used in interrupt_handler.
 * @param pcb: The process will be added.
 * @return: Indicate whether the add action succeeds.
 */
int enqueue_suspend_queue_reversly(PCB *pcb)
{
    // Make pcb the first element of the queue.
    if (pcb)
        return queue_insert_next(SuspendQueue, NULL, pcb);
    else
        return 0;
}

/**
 * Remove the process from suspend queue by given disk id.
 * @param disk_id: The disk id needs to be matched of the process.
 * @return If succeeds, the pcb is returned.
 * If the queue is empty or the operation fails, NULL is returned.
 */
PCB *remove_from_suspend_queue_by_disk_id(INT16 disk_id)
{
    if (!SuspendQueue)
        return NULL;

    PCB *pcb;
    int result;
    QueueElement *element;
    fp_match = match_disk_id;

    element = find_from_queue_by_condition(SuspendQueue, fp_match, (void *) disk_id);
    if (!element)
        return NULL;
    else
    {
        result = queue_remove_element(SuspendQueue, element, (void **) &pcb);
        if (!result)
        {
            error_message("queue_remove_element");
            shut_down();
        }
        return pcb;
    }
}

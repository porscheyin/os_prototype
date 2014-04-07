/*
 * File: os_utils.c
 * Author: Yin Zhang
 * Date: September 11, 2013
 * Description: The header contains the public interfaces
 * of the utility routines and routines relating to interact
 * with hardware which are used by process management and
 * kernel service module.
 */

#ifndef OS_UTILS_H
#define	OS_UTILS_H

#include "base/global.h"
#include "base/protos.h"

// Used by configuration argument.

typedef enum output_state
{
    Full, Limited, None
} OutputState;

// Configuration argument.

typedef struct arg_entry
{
    const char *argument_name;
    void (*entry_point)(void);
    OutputState show_other_output;
    OutputState show_scheduler_output;
    OutputState show_memory_output;
} ConfigArgEntry;

// Used for debugging. Print function name and process id to show process execution stage.
#define stage_info(pcb, message) do                                   \
{                                                                     \
    printf("\nPCB: %d, In %s: %s\n", (pcb)->pid, __func__, (message));\
} while(0)

// Used for debugging. Print function name and message to indicate internal errors.
#define error_message(message) do                                     \
{                                                                     \
    printf("\nInternal Error! In %s: %s\n", __func__, (message));     \
} while(0)

#define SLEEP_PERIOD 100 * 300

#define wait_and_sleep(time) do { usleep((time)); } while(0)


// Description: Get appropriate configuration argument entry according to the input argument.
// Parameter @input_argument_name: The argument from command indicating the entry point of initial process.
// Return: On success, the pointer of the argument entry is returned.
// NULL is returned when there is no matching entry.
ConfigArgEntry *get_config_arg(const char *input_argument_name);

// Description: Wait for a while.
// Parameter: None.
// Return: None.
void wait_for_time();

// Description: When waiting for some process get ready,
// call this function. It will Wait for a while.
// Parameter: None.
// Return: None.
void idle_and_wait(void);

// Description: Get current machine time.
// Parameter: None.
// Return: Current machine time.
INT32 get_current_time(void);

// Description: Start the delay timer.
// Parameter @time: Delay time.
// Return: None.
void start_timer(INT32 *time);

// Description: Get timer status.
// Parameter: None.
// Return: Timer status.
INT32 get_timer_status(void);

// Description: Get the lock.
// Parameter @lock_name: Lock to get.
// Return: On success, 1 is returned.  On error, 0 is returned.
INT32 get_data_lock(INT32 lock_name);

// Description: Release the lock.
// Parameter @lock_name: Lock to release.
// Return: On success, 1 is returned.  On error, 0 is returned.
INT32 release_data_lock(INT32 lock_name);

// Description: Make hardware context.
// Parameter @returning_context: Returned pointer to the newly created context.
// Parameter @starting_address: The entry point address.
// Parameter @user_or_kernel: Mode used when creating context.
// Return: None.
void make_context(void **returning_context, void *starting_address,
        BOOL user_or_kernel);

// Description: Switch to another hardware context.
// Parameter @kill_or_save: Mode used when switch context.
// Parameter @incoming_context: Next context which will be switched to.
// Return: None.
void switch_context(BOOL kill_or_save, void **incoming_context);

// Description: Shutdown the hardware (End the simulation).
// Parameter: None.
// Return: None.
void shut_down();

/**
 * Write the value to a specific position in memory.
 * @param position: The place where to write the value.
 * @param value: The value need to be written to the position.
 *               Here the pointer to the value need to be passed.
 */
void write_to_memory(INT32 position, INT32 *value);

/**
 * Read the value from a specific position in memory.
 * @param position: The place where to read the value.
 * @param value: The value will be stored here. Here the pointer to the value need to be passed.
 */
void read_from_memory(INT32 position, INT32 *value);

#endif	/* OS_UTILS_H */

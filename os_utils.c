/*
 * File: os_utils.c
 * Author: Yin Zhang
 * Date: September 11, 2013
 * Description: This file contains the implementation of
 * the utility routines and routines relating to interact
 * with hardware which are used by process management and
 * kernel service module.
 */

#include <string.h>
#include <unistd.h>
#include "common.h"
#include "base/syscalls.h"
#include "os_utils.h"
#include "proc_mgmt.h"
#include "data_struct.h"

// Used to save global configuration argument.
ConfigArgEntry *ConfigArgument;

// Global configuration argument table, used to save entry_point and output limitations.
ConfigArgEntry config_arg_table[] = {
    { "test0", test0, Full, None, None},
    { "test1a", test1a, Full, None, None},
    { "test1b", test1b, Full, None, None},
    { "test1c", test1c, Limited, Full, None},
    { "test1d", test1d, Limited, Full, None},
    { "test1e", test1e, Full, None, None},
    { "test1f", test1f, Limited, Full, None},
    { "test1g", test1g, Full, None, None},
    { "test1h", test1h, Limited, Full, None},
    { "test2a", test2a, Full, None, Full},
    { "test2b", test2b, Full, None, Full},
    { "test2c", test2c, Limited, Full, None},
    { "test2d", test2d, Limited, Limited, None},
    { "test2e", test2e, Limited, Limited, Limited},
    { "test2f", test2f, Limited, None, Limited},
};

// Description: Get appropriate configuration argument entry according to the input argument.
// Parameter @input_argument_name: The argument from command indicating the entry point of initial process.
// Return: On success, the pointer of the argument entry is returned.
// NULL is returned when there is no matching entry.

ConfigArgEntry *get_config_arg(const char *input_argument_name)
{
    int i;
    ConfigArgEntry *ret;
    size_t size = (sizeof config_arg_table) / (sizeof config_arg_table[0]);
    for (i = 0; i < size; i++)
    {
        if (strcmp(config_arg_table[i].argument_name, input_argument_name) == 0)
            return ret = &config_arg_table[i];
    }
    return ret = NULL;
}

// Description: Wait for a while.
// Parameter: None.
// Return: None.

void wait_for_time(void)
{
    wait_and_sleep(SLEEP_PERIOD);
}

// Description: When waiting for some process get ready,
// call this function. It will Wait for a while.
// Parameter: None.
// Return: None.

void idle_and_wait(void)
{
    CALL(Z502Idle());
    // Don't call Z502Idle() too fast, make sure the event
    // is triggered within ten times of Z502Idle() invocations.
    wait_for_time();
}

// Description: Get current machine time.
// Parameter: None.
// Return: Current machine time.

INT32 get_current_time(void)
{
    INT32 ret;
    INT32 time;
    CALL(MEM_READ(Z502ClockStatus, &time));
    ret = time;
    return ret;
}

// Description: Start the delay timer.
// Parameter @time: Delay time.
// Return: None.

void start_timer(INT32 *time)
{
    CALL(MEM_WRITE(Z502TimerStart, time));
}

// Description: Get timer status.
// Parameter: None.
// Return: Timer status.

INT32 get_timer_status(void)
{
    INT32 ret;
    INT32 status;
    CALL(MEM_READ(Z502TimerStatus, &status));
    ret = status;
    return ret;
}

// Description: Get the lock.
// Parameter @lock_name: Lock to get.
// Return: On success, 1 is returned.  On error, 0 is returned.

INT32 get_data_lock(INT32 lock_name)
{
    INT32 lock_result;
    CALL(READ_MODIFY(lock_name, DO_LOCK, SUSPEND_UNTIL_LOCKED, &lock_result));
    return lock_result;
}

// Description: Release the lock.
// Parameter @lock_name: Lock to release.
// Return: On success, 1 is returned.  On error, 0 is returned.

INT32 release_data_lock(INT32 lock_name)
{
    INT32 lock_result;
    CALL(READ_MODIFY(lock_name, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &lock_result));
    return lock_result;
}

// Description: Make hardware context.
// Parameter @returning_context: Returned pointer to the newly created context.
// Parameter @starting_address: The entry point address.
// Parameter @user_or_kernel: Mode used when creating context.
// Return: None.

void make_context(void **returning_context, void *starting_address,
                  BOOL user_or_kernel)
{
    CALL(Z502MakeContext(returning_context, starting_address, user_or_kernel));
}

// Description: Switch to another hardware context.
// Parameter @kill_or_save: Mode used when switch context.
// Parameter @incoming_context: Next context which will be switched to.
// Return: None.

void switch_context(BOOL kill_or_save, void **incoming_context)
{
    CALL(Z502SwitchContext(kill_or_save, incoming_context));
}

// Description: Shutdown the hardware (End the simulation).
// Before halting, all allocated memory must be freed.
// Parameter: None.
// Return: None.

void shut_down(void)
{
    printf("All processes will be terminated!\n");
    CALL(Z502Halt());
}

/**
 * Write the value to a specific position in memory.
 * @param position: The place where to write the value.
 * @param value: The value need to be written to the position.
 *               Here the pointer to the value need to be passed.
 */
void write_to_memory(INT32 position, INT32 *value)
{
    MEM_WRITE(position, value);
}

/**
 * Read the value from a specific position in memory.
 * @param position: The place where to read the value.
 * @param value: The value will be stored here. Here the pointer to the value need to be passed.
 */
void read_from_memory(INT32 position, INT32 *value)
{
    MEM_READ(position, value);
}

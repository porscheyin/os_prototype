/* 
 * File: common.h
 * Author: Yin Zhang
 * Data: September 21, 2013, 11:53 AM
 * Description: The header contains the macros used by all modules.
 */

#ifndef COMMON_H
#define	COMMON_H

// Used as conditional compilation for debugging.
//#define DEBUG_PROCESS
//#define DEBUG_SCHEDULE
//#define DEBUG_STAGE

// Limitations.
#define MAX_NUMBER_OF_USER_PROCESSES      15
#define MAX_NUMBER_OF_PROCESSE_NAME       32
#define MAX_NUMBER_OF_MESSAGES            10
#define MAX_LENGTH_OF_LEGAL_MESSAGE       64

// Used for scheduler printer setup.
#define ACTION_NAME_ALLDONE    "AllDone"
#define ACTION_NAME_CREATE     "Create"
#define ACTION_NAME_DISPATCH   "Dispatch"
#define ACTION_NAME_DONE       "Done"
#define ACTION_NAME_SUSPEND    "Suspend"
#define ACTION_NAME_READY      "Ready"
#define ACTION_NAME_RESUME     "Resume"
#define ACTION_NAME_WAIT       "Wait"
#define ACTION_NAME_WRITE      "Write"
#define ACTION_NAME_READ       "Read"
#define ACTION_NAME_INTERRUPT  "Interupt"

#define NORMAL_INFO 1
#define FINAL_INFO 0

// Self-defined error code.
#define ERR_DUPLICATE_PROCESS_NAME              8L
#define ERR_ILLEGAL_PRIORITY                    9L
#define ERR_EXCEED_MAX_PROCESS_NUMBER           10L
#define ERR_EXCEED_MAX_PROCESS_NAME_NUMBER      11L
#define ERR_QUEUE_MANIPULATION                  12L
#define ERR_PROCESS_DOESNT_EXIST                13L
#define ERR_SUSPEND_SELF                        14L
#define ERR_ALREADY_SUSPENDED                   15L
#define ERR_RESUME_SELF                         16L
#define ERR_RESUME_UNSUSPENDED_PROCESS          17L
#define ERR_ILLEGAL_MESSAGE_LENGTH              18L
#define ERR_EXCEED_MAX_NUMBER_OF_MESSAGES       19L

// Default priority for the initial process.
#define DEFAULT_PRIORITY 8

// Lock names.
#define COMMON_DATA_LOCK  ((MEMORY_INTERLOCK_BASE) + 1)
#define TIMER_QUEUE_LOCK  ((MEMORY_INTERLOCK_BASE) + 2)
#define READY_QUEUE_LOCK  ((MEMORY_INTERLOCK_BASE) + 3)
#define SUSPEND_QUEUE_LOCK  ((MEMORY_INTERLOCK_BASE) + 4)
#define PRINT_LOCK  ((MEMORY_INTERLOCK_BASE) + 5)

// Used for lock operations.
#define DO_LOCK                                 1
#define DO_UNLOCK                               0
#define SUSPEND_UNTIL_LOCKED                    1
#define DO_NOT_SUSPEND                          0

#define WRITE_ONE 0
#define WRITE_TWO 1
#define READ_ONE 2
#define READ_TWO 3

#endif	/* COMMON_H */

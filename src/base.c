/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Modified by Yin Zhang. October 15th, 2013
 ************************************************************************/

#include "base/global.h"
#include "base/syscalls.h"
#include "base/protos.h"
#include "string.h"
#include "common.h"
#include "os_utils.h"
#include "proc_mgmt.h"
#include "data_struct.h"
#include "storage_mgmt.h"

extern void *TO_VECTOR[];
extern long Z502_REG3;
extern PCB *RootPCB;
extern PCB *CurrentPCB;
extern ConfigArgEntry *ConfigArgument;
extern char MEMORY[PHYS_MEM_PGS * PGSIZE ];

char *call_names[] = {"mem_read ", "mem_write", "read_mod ", "get_time ", "sleep    ", "get_pid  ",
    "create   ", "term_proc", "suspend  ", "resume   ", "ch_prior ",
    "send     ", "receive  ", "disk_read", "disk_wrt ", "def_sh_ar"};

extern UINT16 *shadow_pg_tbl[PHYS_MEM_PGS];
extern UINT16 process_holder[PHYS_MEM_PGS];
extern UINT16 *address_holder[10];
extern int shadow_pg_idx;
extern int ref_idx;

/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.
 ************************************************************************/
void interrupt_handler(void)
{
    INT32 status;
    INT32 device_id;
    INT32 Index = 0;

    static BOOL remove_this_in_your_code = TRUE;
    static INT32 how_many_interrupt_entries = 0;

    // Get cause of interrupt
    read_from_memory(Z502InterruptDevice, &device_id);
    // Set this device as target of our query
    write_to_memory(Z502InterruptDevice, &device_id);
    // Now read the status of this device
    read_from_memory(Z502InterruptStatus, &status);

    // Conditional output.
    if (ConfigArgument->show_other_output == Full)
    {
        printf("Interrupt_handler: Found device ID %d with status %d\n",
               device_id, status);
    }
    else if (ConfigArgument->show_other_output == Limited)
    {
        how_many_interrupt_entries++; /** TEMP **/
        if (remove_this_in_your_code && (how_many_interrupt_entries < 10))
        {
            printf("Interrupt_handler: Found device ID %d with status %d\n",
                   device_id, status);
        }
    }
    
    if (device_id == 4)
    {
        os_make_ready_to_run();
    }
    else if (device_id >= 5 || device_id <= 7)
    {
        read_write_scheduler(device_id);
    }

    Index = 0;
    write_to_memory(Z502InterruptClear, &Index);
    read_from_memory(Z502InterruptDevice, &device_id);
    // Clear out this device - we're done with it
    write_to_memory(Z502InterruptClear, &Index);
} /* End of interrupt_handler */

/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void fault_handler(void)
{
    INT32 device_id;
    INT32 status;
    INT32 Index = 0;
    static INT32 how_many_interrupt_entries = 0;

    // Get cause of interrupt
    read_from_memory(Z502InterruptDevice, &device_id);
    // Set this device as target of our query
    write_to_memory(Z502InterruptDevice, &device_id);
    // Now read the status of this device
    read_from_memory(Z502InterruptStatus, &status);

    if (ConfigArgument->show_other_output == Full)
    {
        printf("Fault_handler: Found vector type %d with value %d\n", device_id, status);
    }
    else if (ConfigArgument->show_other_output == Limited)
    {
        how_many_interrupt_entries++;
        if (how_many_interrupt_entries < 10)
        {
            printf("Fault_handler: Found vector type %d with value %d\n", device_id, status);
        }
    }

    if (status >= 1024)
    {
        printf("Invalid address: virtual page number out of range!\n");
        shut_down();
    }

    frame_scheduler(status);

    // Conditional output.
    if (ConfigArgument->show_memory_output == Full)
    {
        int idx;
        for (idx = 0; idx < NUM_OF_FRAMES; idx++)
        {
            if (shadow_pg_tbl[idx] != NULL)
            {
                MP_setup((INT32) ((*shadow_pg_tbl[idx]) & PTBL_FRAME_BITS), (INT32) process_holder[idx],
                         (INT32) (shadow_pg_tbl[idx] - address_holder[process_holder[idx]]),
                         (INT32) ((*shadow_pg_tbl[idx]) & PTBL_STATE_BITS) >> 13);
            }
        }
        MP_print_line();
    }
    else if (ConfigArgument->show_memory_output == Limited)
    {
        how_many_interrupt_entries++;
        if (how_many_interrupt_entries < 10)
        {
            int idx;
            for (idx = 0; idx < NUM_OF_FRAMES; idx++)
            {
                if (shadow_pg_tbl[idx] != NULL)
                {
                    MP_setup((INT32) ((*shadow_pg_tbl[idx]) & PTBL_FRAME_BITS), (INT32) process_holder[idx],
                             (INT32) (shadow_pg_tbl[idx] - address_holder[process_holder[idx]]),
                             (INT32) ((*shadow_pg_tbl[idx]) & PTBL_STATE_BITS) >> 13);
                }
            }
            MP_print_line();
        }
    }
    // Clear out this device - we're done with it
    write_to_memory(Z502InterruptClear, &Index);
} /* End of fault_handler */

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA * SystemCallData)
{
    short i;
    PCB *pcb;
    short call_type;
    static short do_print = 10;

    call_type = (short) SystemCallData->SystemCallNumber;

    // Conditional output according to student manual.
    if (ConfigArgument->show_other_output == Full)
    {
        printf("SVC handler: %s\n", call_names[call_type]);
        for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++)
        {
            printf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
                   (unsigned long) SystemCallData->Argument[i],
                   (unsigned long) SystemCallData->Argument[i]);
        }
    }
    else if (ConfigArgument->show_other_output == Limited)
    {
        if (do_print > 0)
        {
            printf("SVC handler: %s\n", call_names[call_type]);
            for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++)
            {
                printf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
                       (unsigned long) SystemCallData->Argument[i],
                       (unsigned long) SystemCallData->Argument[i]);
            }
            do_print--;
        }
    }

    switch (call_type)
    {
        case SYSNUM_GET_TIME_OF_DAY: // Get time service call.
        {
            *SystemCallData->Argument[0] = (long) get_current_time();
            break;
        }
        case SYSNUM_SLEEP:
        {
            os_process_sleep((long) SystemCallData->Argument[0]);
            os_dispatcher();
            break;
        }
        case SYSNUM_CREATE_PROCESS:
        {
            pcb = os_create_process((const char *) SystemCallData->Argument[0],
                                    (void *) SystemCallData->Argument[1],
                                    (INT32) SystemCallData->Argument[2],
                                    SystemCallData->Argument[4]);
            if (pcb)
            {
                *SystemCallData->Argument[3] = (long) pcb->pid;
                if (pcb)
                {
                    print_scheduling_info(ACTION_NAME_CREATE, pcb, NORMAL_INFO);
                }
            }
            else
            {
                *SystemCallData->Argument[3] = -1;
            }
            break;
        }
        case SYSNUM_GET_PROCESS_ID:
        {
            os_get_process_id((const char *) SystemCallData->Argument[0],
                              (INT32 *) SystemCallData->Argument[1],
                              SystemCallData->Argument[2]);
            break;
        }
        case SYSNUM_SUSPEND_PROCESS:
        {
            os_suspend_process((INT32) SystemCallData->Argument[0],
                               SystemCallData->Argument[1]);
            break;
        }
        case SYSNUM_RESUME_PROCESS:
        {
            os_resume_process((INT32) SystemCallData->Argument[0],
                              SystemCallData->Argument[1]);
            break;
        }
        case SYSNUM_CHANGE_PRIORITY:
        {
            os_change_priority((INT32) SystemCallData->Argument[0],
                               (INT32) SystemCallData->Argument[1],
                               SystemCallData->Argument[2]);
            break;
        }
        case SYSNUM_TERMINATE_PROCESS:
        {
            if (ConfigArgument->entry_point == test0)
            {
                shut_down();
            }
            else
            {
                os_terminate_process((INT32) SystemCallData->Argument[0],
                                     SystemCallData->Argument[1]);
            }
            break;
        }
        case SYSNUM_DISK_READ:
        {
            os_disk_read((INT32) SystemCallData->Argument[0],
                         (INT32) SystemCallData->Argument[1],
                         (char *) SystemCallData->Argument[2]);

            break;
        }
        case SYSNUM_DISK_WRITE:
        {
            os_disk_write((INT32) SystemCallData->Argument[0],
                          (INT32) SystemCallData->Argument[1],
                          (char *) SystemCallData->Argument[2]);
            break;
        }
        default:
        {
            printf("ERROR!  call_type not recognized!\n");
            printf("Call_type is - %i\n", call_type);
            break;
        }
    } // End of switch
} // End of svc

/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[])
{
    INT32 i;
    PCB *pcb;
    long error;
    void *next_context;

    /* Demonstrates how calling arguments are passed thru to here       */

    printf("Program called with %d arguments:", argc);
    for (i = 0; i < argc; i++)
        printf(" %s", argv[i]);
    printf("\n");

    /*          Setup so handlers will come to code in base.c           */
    TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR ] = (void *) interrupt_handler;
    TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR ] = (void *) fault_handler;
    TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR ] = (void *) svc;

    if (!init_queues())
    {
        error_message("Queues initialization fails!");
        return;
    }

    init_process_table();

    init_storage();

    if ((argc > 1) && ((ConfigArgument = get_config_arg(argv[1])) != NULL))
    {
        /*  Determine if the switch was set, and if so go to demo routine.  */
        if (strcmp(argv[1], "sample") == 0 || strcmp(argv[1], "test0") == 0)
        {
            make_context(&next_context, ConfigArgument->entry_point, KERNEL_MODE);
            switch_context(SWITCH_CONTEXT_KILL_MODE, &next_context);
        }
        else
        {
            pcb = os_create_process("initial_process",
                                    ConfigArgument->entry_point,
                                    DEFAULT_PRIORITY, &error);
            if (pcb)
            {
                CurrentPCB = RootPCB = pcb;
                if (ConfigArgument->show_scheduler_output == Full
                        || ConfigArgument->show_scheduler_output == Limited)
                {
                    CALL(SP_setup_action(SP_ACTION_MODE, "Create"));
                    CALL(SP_setup(SP_TARGET_MODE, (INT32) CurrentPCB->pid));
                    CALL(SP_setup(SP_NEW_MODE, CurrentPCB->pid));
                    CALL(SP_setup(SP_RUNNING_MODE, CurrentPCB->pid));
                    CALL(SP_print_header());
                    CALL(SP_print_line());
                }
                switch_context(SWITCH_CONTEXT_SAVE_MODE, &pcb->context);
            }
            else
            {
                printf("os_create_initial_process: creation of pcb fails");
                return;
            }
        }
    }
} // End of osInit

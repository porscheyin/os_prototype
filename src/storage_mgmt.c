#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "common.h"
#include "base/global.h"
#include "base/protos.h"
#include "os_utils.h"
#include "base/syscalls.h"
#include "proc_mgmt.h"
#include "data_struct.h"
#include "storage_mgmt.h"

Queue *DiskQueue;
Queue *FrameQueue;
extern PCB *CurrentPCB;
extern Queue *SuspendQueue;
extern FuncMatch fp_match;
extern UINT16 *Z502_PAGE_TBL_ADDR;
extern INT16 Z502_PAGE_TBL_LENGTH;
extern char MEMORY[PHYS_MEM_PGS * PGSIZE];
extern long Z502_REG3;
UINT16 *shadow_pg_tbl[PHYS_MEM_PGS];
UINT16 process_holder[PHYS_MEM_PGS];
UINT16 *address_holder[10];
int shadow_pg_idx = -1;
int ref_idx = -1;

/**
 * Initialize the frame queue and shallow page table.
 */
void init_storage(void)
{
    int i = 0;
    for (i = 0; i < PHYS_MEM_PGS; i++)
    {
        add_to_frame_queue((INT16) i);
        shadow_pg_tbl[i] = NULL;
    }
}

/**
 * Create a frame and add it to the frame queue.
 * @param frame_number: Used as the id of the newly created frame.
 * @return: The value indicates whether the action succeeds.
 */
int add_to_frame_queue(INT16 frame_number)
{
    Frame *frm = malloc(sizeof (*frm));
    if (!frm)
        return 0;
    frm->frame_number = frame_number;
    return queue_enqueue(FrameQueue, frm);
}

/**
 * Get the number of the newly removed frame from free frame queue.
 * @return: The number of the frame, if there is no more free frame, -1 is returned.
 */
INT16 get_frame_number_of_removed_frame(void)
{
    Frame *frm = removed_from_frame_queue();
    return frm ? frm->frame_number : -1;
}

/**
 * Remove a frame form the frame queue.
 * @return: If the queue is empty, NULL is returned, or the pointer to the frame is returned.
 */
Frame *removed_from_frame_queue(void)
{
    Frame *frm;

    if (!FrameQueue->size)
    {
        return NULL;
    }
    else
    {
        queue_dequeue(FrameQueue, (void**) &frm);
        return frm;
    }
}

/**
 * Write data into the specific position indicated by the disk id and sector id.
 * @param disk_id: Indicates which disk to write to. 
 * @param sector: Indicates which sector to write to. 
 * @param buffer: The data needs to be written.
 */
void os_disk_write(INT32 disk_id, INT32 sector, char *buffer)
{
    INT32 status;
    int result;

    /* Do the hardware call to put data on disk */
    write_to_memory(Z502DiskSetID, &disk_id);
    read_from_memory(Z502DiskStatus, &status);

    // If the disk is free, indicates success in writing.
    if (status == DEVICE_FREE)
    {
        write_to_memory(Z502DiskSetSector, &sector);
        write_to_memory(Z502DiskSetBuffer, (INT32 *) buffer);
        status = 1; // Specify a write
        write_to_memory(Z502DiskSetAction, &status);
        status = 0; // Must be set to 0
        write_to_memory(Z502DiskStart, &status);

        CurrentPCB->disk_id = disk_id;
        CurrentPCB->operation = -1;

        get_data_lock(SUSPEND_QUEUE_LOCK);
        result = add_to_suspend_queue(CurrentPCB);
        release_data_lock(SUSPEND_QUEUE_LOCK);
        if (result)
        {
            CurrentPCB->suspend = TRUE;
            print_scheduling_info(ACTION_NAME_SUSPEND, CurrentPCB, NORMAL_INFO);
        }
        else
        {
            error_message("add_to_suspend_queue");
            shut_down();
        }
        os_dispatcher();
        return;
    }
    // If the disk is busy, indicates failure in writing.
    else if (status == DEVICE_IN_USE)
    {
        DISK_DATA *disk_data;
        disk_data = (DISK_DATA *) calloc(1, sizeof ( DISK_DATA));

        CurrentPCB->disk_id = disk_id;
        CurrentPCB->operation = WRITE_ONE;
        CurrentPCB->disk = disk_id;
        CurrentPCB->sector = sector;
        memcpy(disk_data, buffer, sizeof (DISK_DATA));
        CurrentPCB->disk_data = disk_data;
        result = add_to_suspend_queue(CurrentPCB);
        if (result)
        {
            CurrentPCB->suspend = TRUE;
            print_scheduling_info(ACTION_NAME_WRITE, CurrentPCB, NORMAL_INFO);
        }
        else
        {
            error_message("add_to_suspend_queue");
            shut_down();
        }
        os_dispatcher();
        return;
    }
    else
    {
        printf("some error not processed in os_disk_write!\n");
    }
}

/**
 * Read data from the specific position indicated by the disk id and sector id.
 * @param disk_id: Indicates which disk to read from. 
 * @param sector: Indicates which sector to read from. 
 * @param buffer: The buffer to hold the data.
 */
void os_disk_read(INT32 disk_id, INT32 sector, char *buffer)
{
    INT32 status;
    int result;

    write_to_memory(Z502DiskSetID, &disk_id);
    read_from_memory(Z502DiskStatus, &status);
    // Disk hasn't been used - should be free
    if (status == DEVICE_FREE)
    {
        write_to_memory(Z502DiskSetSector, &sector);
        write_to_memory(Z502DiskSetBuffer, (INT32 *) buffer);
        status = 0; // Specify a read
        write_to_memory(Z502DiskSetAction, &status);
        status = 0; // Must be set to 0
        write_to_memory(Z502DiskStart, &status);

        CurrentPCB->disk_id = disk_id;
        CurrentPCB->operation = -1;

        get_data_lock(SUSPEND_QUEUE_LOCK);
        result = add_to_suspend_queue(CurrentPCB);
        release_data_lock(SUSPEND_QUEUE_LOCK);
        if (result)
        {
            CurrentPCB->suspend = TRUE;
            print_scheduling_info(ACTION_NAME_READ, CurrentPCB, NORMAL_INFO);
        }
        else
        {
            error_message("add_to_suspend_queue");
            shut_down();
        }
        os_dispatcher();
    }
    else if (status == DEVICE_IN_USE)
    {
        CurrentPCB->disk_id = disk_id;
        CurrentPCB->operation = READ_ONE;
        CurrentPCB->disk = disk_id;
        CurrentPCB->sector = sector;
        CurrentPCB->disk_data = (DISK_DATA *) buffer;

        get_data_lock(SUSPEND_QUEUE_LOCK);
        result = add_to_suspend_queue(CurrentPCB);
        release_data_lock(SUSPEND_QUEUE_LOCK);
        if (result)
        {
            CurrentPCB->suspend = TRUE;
            print_scheduling_info(ACTION_NAME_READ, CurrentPCB, NORMAL_INFO);
        }
        else
        {
            error_message("add_to_suspend_queue");
            shut_down();
        }
        os_dispatcher();
    }
}

/**
 * Used for interrupt handler. According to the action the process wants to take,
 * do the corresponding work and call dispatcher to schedule the processes.
 * @param device_id: The id of the device obtained from interrupt handler.
 */
void read_write_scheduler(INT32 device_id)
{
    PCB *pcb;
    INT16 disk_id;

    switch (device_id)
    {
        case 5:
            disk_id = 1;
            break;
        case 6:
            disk_id = 2;
            break;
        case 7:
            disk_id = 3;
            break;
        default:
            error_message("Illegal device id.");
            break;
    }

    pcb = remove_from_suspend_queue_by_disk_id(disk_id);

    if (pcb != NULL)
    {
        // If the process needs to write, then write data to specific sector.
        if (pcb->operation == WRITE_ONE)
        {
            INT32 status;
            
            write_to_memory(Z502DiskSetID, & pcb->disk);
            read_from_memory(Z502DiskStatus, &status);
            if (status == DEVICE_FREE)
            {
                write_to_memory(Z502DiskSetSector, & pcb->sector);
                write_to_memory(Z502DiskSetBuffer, (INT32 *) pcb->disk_data);
                status = 1; // Specify a write.
                write_to_memory(Z502DiskSetAction, &status);
                status = 0; // Must be set to 0.
                write_to_memory(Z502DiskStart, &status);
                pcb->operation = WRITE_TWO;
                enqueue_suspend_queue_reversly(pcb);
                pcb->suspend = TRUE;
                print_scheduling_info(ACTION_NAME_WRITE, pcb, NORMAL_INFO);
            }
            else
            {
                enqueue_suspend_queue_reversly(pcb);
                pcb->suspend = TRUE;
                print_scheduling_info(ACTION_NAME_WRITE, pcb, NORMAL_INFO);
            }
        }

            // If the process needs to read, then read data from specific sector.
        else if (pcb->operation == READ_ONE)
        {
            INT32 status;

            write_to_memory(Z502DiskSetID, & pcb->disk);
            read_from_memory(Z502DiskStatus, &status);
            if (status == DEVICE_FREE)
            {
                write_to_memory(Z502DiskSetSector, & pcb->sector);
                write_to_memory(Z502DiskSetBuffer, (INT32 *) pcb->disk_data);
                status = 0; // Specify a read.
                write_to_memory(Z502DiskSetAction, &status);
                status = 0; // Must be set to 0.
                write_to_memory(Z502DiskStart, &status);
                pcb->operation = READ_TWO;
                enqueue_suspend_queue_reversly(pcb);
                pcb->suspend = TRUE;
                print_scheduling_info(ACTION_NAME_READ, pcb, NORMAL_INFO);
            }
            else
            {
                // If in use, add it reversely.
                enqueue_suspend_queue_reversly(pcb);
                pcb->suspend = TRUE;
                print_scheduling_info(ACTION_NAME_READ, pcb, NORMAL_INFO);
            }
        }
        else
        {
            add_to_ready_queue(pcb);
            pcb->suspend = FALSE;
            print_scheduling_info(ACTION_NAME_READY, pcb, NORMAL_INFO);
        }
    }
}

/**
 * Used in fault handler. Deal with the page fault and map the pages to the frames.
 * @param status: The virtual page number obtained from fault handler.
 */
void frame_scheduler(INT32 status)
{
    int pid;
    int disk_id;
    INT16 offset;
    INT32 Index = 0;
    INT16 available_frame;
    offset = Z502_REG3 % PGSIZE;
    disk_id = CurrentPCB->pid + 1;

    if (!Z502_PAGE_TBL_ADDR)
    {
        Z502_PAGE_TBL_LENGTH = 1024;
        Z502_PAGE_TBL_ADDR = (UINT16 *) calloc(Z502_PAGE_TBL_LENGTH, sizeof (UINT16));
        address_holder[disk_id - 1] = Z502_PAGE_TBL_ADDR;
    }

    // If page is invalid and not in the disk.
    if (!(Z502_PAGE_TBL_ADDR[status] & PTBL_VALID_BIT))
    {
        if (!(Z502_PAGE_TBL_ADDR[status] & PTBL_RESERVED_BIT))
        {
            available_frame = get_frame_number_of_removed_frame();
            if (available_frame >= 0)
            {
                Z502_PAGE_TBL_ADDR[status] = available_frame | PTBL_VALID_BIT;
                shadow_pg_idx++;
                shadow_pg_tbl[shadow_pg_idx] = &Z502_PAGE_TBL_ADDR[status];
                process_holder[shadow_pg_idx] = disk_id - 1;
            }
            else
            {
                for (;;)
                {
                    ref_idx = (ref_idx + 1) % PHYS_MEM_PGS;
                    ref_idx++;
                    if (*shadow_pg_tbl[ref_idx] & PTBL_REFERENCED_BIT)
                    {
                        *shadow_pg_tbl[ref_idx] &= ~PTBL_REFERENCED_BIT;
                    }
                    else
                    {
                        short frame_number;
                        frame_number = (short) (*shadow_pg_tbl[ref_idx]) & PTBL_FRAME_BITS;
                        write_to_memory(Z502InterruptClear, &Index);
                        pid = process_holder[ref_idx];
                        os_disk_write(pid + 1, shadow_pg_tbl[ref_idx] - address_holder[pid],
                                      (char *) &MEMORY[frame_number * PGSIZE]);
                        *shadow_pg_tbl[ref_idx] |= PTBL_RESERVED_BIT;
                        *shadow_pg_tbl[ref_idx] &= ~PTBL_VALID_BIT;
                        Z502_PAGE_TBL_ADDR[status] = frame_number | PTBL_VALID_BIT;
                        shadow_pg_tbl[ref_idx] = &Z502_PAGE_TBL_ADDR[status];
                        process_holder[ref_idx] = disk_id - 1;
                        break;
                    }
                }
            }
        }
        else
        {
            available_frame = get_frame_number_of_removed_frame();
            if (available_frame >= 0)
            {
                os_disk_read(disk_id, status, (char *) &MEMORY[available_frame * PGSIZE]);
                Z502_PAGE_TBL_ADDR[status] = available_frame | PTBL_VALID_BIT;
                shadow_pg_idx++;
                shadow_pg_tbl[shadow_pg_idx] = &Z502_PAGE_TBL_ADDR[status];
                process_holder[shadow_pg_idx] = disk_id - 1;
            }
            else
            {
                for (;;)
                {
                    ref_idx = (ref_idx + 1) % PHYS_MEM_PGS;
                    ref_idx++;
                    if (*shadow_pg_tbl[ref_idx] & PTBL_REFERENCED_BIT)
                    {
                        *shadow_pg_tbl[ref_idx] &= ~PTBL_REFERENCED_BIT;
                    }
                    else
                    {
                        // Choose this as victim.
                        short frame_number;
                        frame_number = (short) (*shadow_pg_tbl[ref_idx]) & PTBL_FRAME_BITS;
                        write_to_memory(Z502InterruptClear, &Index);
                        pid = process_holder[ref_idx];
                        os_disk_write(pid + 1, shadow_pg_tbl[ref_idx] - address_holder[pid],
                                      (char *) &MEMORY[frame_number * PGSIZE]);
                        (*shadow_pg_tbl[ref_idx]) |= PTBL_RESERVED_BIT;
                        (*shadow_pg_tbl[ref_idx]) &= ~PTBL_VALID_BIT;
                        os_disk_read(disk_id, status, (char *) &MEMORY[frame_number * PGSIZE]);
                        Z502_PAGE_TBL_ADDR[status] = frame_number | PTBL_VALID_BIT;
                        shadow_pg_tbl[ref_idx] = &Z502_PAGE_TBL_ADDR[status];
                        process_holder[ref_idx] = disk_id - 1;
                        break;
                    }
                }
            }
        }
    }

    // If next page is also invalid and not in the disk.
    if (!(Z502_PAGE_TBL_ADDR[status + 1] & PTBL_VALID_BIT) && (offset > PGSIZE - 4))
    {
        if (!(Z502_PAGE_TBL_ADDR[status + 1] & PTBL_RESERVED_BIT))
        {
            available_frame = get_frame_number_of_removed_frame();
            if (available_frame >= 0)
            {
                Z502_PAGE_TBL_ADDR[status + 1] = available_frame | PTBL_VALID_BIT;
                shadow_pg_idx++;
                shadow_pg_tbl[shadow_pg_idx] = &Z502_PAGE_TBL_ADDR[status + 1];
                process_holder[shadow_pg_idx] = disk_id - 1;
            }
            else
            {
                for (;;)
                {
                    ref_idx = (ref_idx + 1) % PHYS_MEM_PGS;
                    ref_idx++;
                    if ((*shadow_pg_tbl[ref_idx]) & PTBL_REFERENCED_BIT)
                    {
                        *shadow_pg_tbl[ref_idx] &= ~PTBL_REFERENCED_BIT;
                    }
                    else
                    {
                        short frame_number;
                        frame_number = (short) (*shadow_pg_tbl[ref_idx]) & PTBL_FRAME_BITS;
                        write_to_memory(Z502InterruptClear, &Index);
                        pid = process_holder[ref_idx];
                        os_disk_write(pid + 1, shadow_pg_tbl[ref_idx] - address_holder[pid],
                                      (char *) &MEMORY[frame_number * PGSIZE]);
                        (*shadow_pg_tbl[ref_idx]) |= PTBL_RESERVED_BIT;
                        (*shadow_pg_tbl[ref_idx]) &= ~PTBL_VALID_BIT;
                        Z502_PAGE_TBL_ADDR[status + 1] = frame_number | PTBL_VALID_BIT;
                        shadow_pg_tbl[ref_idx] = &Z502_PAGE_TBL_ADDR[status + 1];
                        process_holder[ref_idx] = disk_id - 1;
                        break;
                    }
                }
            }
        }
        else
        {
            // In the disk.
            available_frame = get_frame_number_of_removed_frame();
            if (available_frame >= 0) // Have a free frame.
            {
                write_to_memory(Z502InterruptClear, &Index);
                os_disk_read(disk_id, status + 1, (char *) &MEMORY[available_frame * PGSIZE]);
                Z502_PAGE_TBL_ADDR[status + 1] = available_frame | PTBL_VALID_BIT;
                shadow_pg_idx++;
                shadow_pg_tbl[shadow_pg_idx] = &Z502_PAGE_TBL_ADDR[status + 1];
                process_holder[shadow_pg_idx] = disk_id - 1;
            }
            else // No more free frame.
            {
                for (;;)
                {
                    ref_idx = (ref_idx + 1) % PHYS_MEM_PGS;
                    ref_idx++;
                    if (*shadow_pg_tbl[ref_idx] & PTBL_REFERENCED_BIT)
                    {
                        // Set the bit as zero;
                        *shadow_pg_tbl[ref_idx] &= ~PTBL_REFERENCED_BIT;
                    }
                    else
                    {
                        short frame_number;
                        frame_number = (short) (*shadow_pg_tbl[ref_idx]) & PTBL_FRAME_BITS;
                        write_to_memory(Z502InterruptClear, &Index);
                        pid = process_holder[ref_idx];
                        os_disk_write(pid + 1, shadow_pg_tbl[ref_idx] - address_holder[pid], (char *) &MEMORY[frame_number * PGSIZE]);
                        (*shadow_pg_tbl[ref_idx]) |= PTBL_RESERVED_BIT;
                        (*shadow_pg_tbl[ref_idx]) &= ~PTBL_VALID_BIT;
                        os_disk_read(disk_id, status + 1, (char *) &MEMORY[frame_number * PGSIZE]);
                        Z502_PAGE_TBL_ADDR[status + 1] = frame_number | PTBL_VALID_BIT;
                        shadow_pg_tbl[ref_idx] = &Z502_PAGE_TBL_ADDR[status + 1];
                        process_holder[ref_idx] = disk_id - 1;
                        break;
                    }
                }
            }
        }
    }
}

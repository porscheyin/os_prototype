/* 
 * File:   storage_mgmt.h
 * Author: yin
 * 
 * Created on November 18, 2013, 6:25 PM
 */

#ifndef STORAGE_MGMT_H
#define	STORAGE_MGMT_H

#include "base/global.h"

#define PTBL_RESERVED_BIT  0x1000
#define PTBL_STATE_BITS    0xE000
#define PTBL_FRAME_BITS    0x0FFF
#define NUM_OF_FRAMES      64

typedef struct disk
{
    INT16 disk_id;
    INT32 pid;
} Disk;

typedef struct frame
{
    INT16 frame_number;
} Frame;

typedef union
{
    char char_data[PGSIZE ];
    UINT32 int_data[PGSIZE / sizeof (int)];
} DISK_DATA;

/**
 * Initialize the frame queue and shallow page table.
 */
void init_storage(void);

/**
 * Create a frame and add it to the frame queue.
 * @param frame_number: Used as the id of the newly created frame.
 * @return: The value indicates whether the action succeeds.
 */
int add_to_frame_queue(INT16 frame_number);

/**
 * Get the number of the newly removed frame from free frame queue.
 * @return: The number of the frame, if there is no more free frame, -1 is returned.
 */
INT16 get_frame_number_of_removed_frame(void);

/**
 * Remove a frame form the frame queue.
 * @return: If the queue is empty, NULL is returned, or the pointer to the frame is returned.
 */
Frame *removed_from_frame_queue(void);

/**
 * Write data into the specific position indicated by the disk id and sector id.
 * @param disk_id: Indicates which disk to write to. 
 * @param sector: Indicates which sector to write to. 
 * @param buffer: The data needs to be written.
 */
void os_disk_write(INT32 disk_id, INT32 sector, char *buffer);

/**
 * Read data from the specific position indicated by the disk id and sector id.
 * @param disk_id: Indicates which disk to read from. 
 * @param sector: Indicates which sector to read from. 
 * @param buffer: The buffer to hold the data.
 */
void os_disk_read(INT32 disk_id, INT32 sector, char *buffer);

/**
 * Used for interrupt handler. According to the action the process wants to take,
 * do the corresponding work and call dispatcher to schedule the processes.
 * @param device_id: The id of the device obtained from interrupt handler.
 */
void read_write_scheduler(INT32 device_id);

/**
 * Used in fault handler. Deal with the page fault and map the pages to the frames.
 * @param status: The virtual page number obtained from fault handler.
 */
void frame_scheduler(INT32 status);

#endif	/* STORAGE_MGMT_H */

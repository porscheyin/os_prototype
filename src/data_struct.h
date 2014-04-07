/* 
 * File: data_structure.h
 * Author: Yin Zhang
 * Data: September 21, 2013, 11:53 AM
 * Description: The header contains the public interfaces of
 * the common data structures which are used by process
 * management module.
 */

#ifndef DATA_STRUCT_H
#define	DATA_STRUCT_H

#include <stdlib.h>

// Used as the order when insert and sort the linked list.
#define ASCENDING 1
#define DESCENDING 0

//Define a structure for linked list elements.

typedef struct list_element
{
    void *data;
    struct list_element *next;
} ListElement;

//Define a structure for linked lists.

typedef struct list
{
    size_t size;
    ListElement *head;
    ListElement *tail;
} List;

typedef int (*FuncMatch)(const void *data1, const void *data2);

typedef int (*FuncCompare)(const void *data1, const void *data2);

typedef void (*FuncFreeData)(void *data);

//Public Interface for manipulating linked lists.

// Description: Create and initialize a linked list.
// Parameter: None.
// Return: The newly created linked list. If creation fails, NULL is returned.
List *list_create(void);

// Description: Free a linked list.
// Parameter @list: The list to free.
// Parameter @free_data: The callback used to free data of the list.
// Return: None.
void list_free(List *list, FuncFreeData free_data);

// Description: Insert an new element after the given one using the data given into a list.
// Parameter @list: The list where the data will be inserted into.
// Parameter @element: The element of the list where the data should be inserted after.
// Parameter @data: The data will be inserted.
// Return: On success, 1 is returned.  On error, 0 is returned.
int list_insert_next(List *list, ListElement *element, const void *data);

// Description: Insert a new element to a list in certain order.
// Parameter @list: The list where the data will be inserted into.
// Parameter @compare: The callback used to compare data in the element.
// Parameter @order: The order when inserting data into the list. 1 indicates ascending, 0 means descending.
// Parameter @data: The data will be inserted.
// Return: On success, 1 is returned.  On error, 0 is returned.
int list_insert_orderly(List *list, FuncCompare compare, int order,
        const void *data);

// Description: Remove an element after the given one from a list .
// Parameter @list: The list where the data will be removed from.
// Parameter @element: The element where the data will be removed after
// Parameter @data: Data returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int list_remove_next(List *list, ListElement *element, void **data);

// Description: Find an element in a list.
// Parameter @list: The list where the data will be found.
// Parameter @equal: The callback used to match data in the element.
// Parameter @data: The data will be found.
// Return: On success, the pointer of the element is returned.  On failure, NULL is returned.
ListElement *list_find_element(List *list, FuncMatch equal, const void *data);

// Description: Remove an element from a list.
// Parameter @list: The list where the data will be removed from.
// Parameter @element: The element where the data will be removed.
// Parameter @data: Data returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int list_remove_element(List *list, ListElement *element, void **data);

// Description: Sort a list in a certain order.
// Parameter @list: The list to be sorted.
// Parameter @compare: The callback used to compare the data of the elements in the list.
// Parameter @order: 1 indicates ascending, 0 indicates descending.
// Return: None.
void list_sort(List *list, FuncCompare compare, int order);

// Description: Check if a list contains an element with the given data.
// Parameter @list: The list is to check.
// Parameter @equal: The callback used to match the data of the element.
// Parameter @data: The data to be found.
// Return: 1 indicates contains. 0 indicates not contain.
int list_contains(List *list, FuncMatch equal, const void *data);

// Description: Get the size of a list.
// Parameter @list: The list to get size of.
// Return: The size of the list.
size_t list_size(List * list);

// Description: Check if a list is empty.
// Parameter @list: The list to check.
// Return: 1 indicates empty, 0 indicates not empty.
int list_is_empty(List *list);

// Description: Get the head of a list.
// Parameter @list: The list to get head of.
// Return: The head of the list.
ListElement * list_head(List * list);

// Description: Get the tail of a list.
// Parameter @list: The list to get tail of.
// Return: The tail of the list.
ListElement *list_tail(List *list);

// Description: Check if an element list is the head of a list.
// Parameter @list: The list to check.
// Parameter @element: The element to check.
// Return: 1 indicates the element is the head, 0 indicates the element is the not head.
int list_is_head(List *list, ListElement *element);

// Description: Check if an element list is the tail of a list.
// Parameter @list: The list to check.
// Parameter @element: The element to check.
// Return: 1 indicates the element is the tail, 0 indicates the element is the not tail.
int list_is_tail(ListElement *element);

// Description: Get the data of an element.
// Parameter @list: The element to get data of.
// Return: The data of the element.
void *list_data(ListElement *element);

// Description: Get the next element of a list.
// Parameter @list: The list to get next element of.
// Return: The next element of the list.
ListElement *list_next(ListElement *element);

//Implement queues as linked lists.
typedef List Queue;

typedef ListElement QueueElement;

//Public Interface for manipulating queues.
#define queue_create list_create

#define queue_free list_free

#define queue_insert_next list_insert_next

#define queue_find_element list_find_element

#define queue_remove_element list_remove_element

#define queue_sort list_sort

#define queue_contains list_contains

#define queue_head list_head

#define queue_tail list_tail

#define queue_size list_size

#define queue_is_empty list_is_empty

#define queue_is_head list_is_head

#define queue_is_tail list_is_tail

#define queue_data list_data

#define queue_next list_next

#define queue_enqueue_orderly list_insert_orderly

// Description: Enqueue an element to a queue.
// Parameter @queue: The queue to enqueue.
// Parameter @data: The data of the newly enqueued element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int queue_enqueue(Queue *queue, const void *data);

// Description: Dequeue an element from a queue.
// Parameter @queue: The queue to dequeue.
// Parameter @data: The data returned from the dequeued element.
// Return: On success, 1 is returned.  On error, 0 is returned.
int queue_dequeue(Queue *queue, void **data);

#endif	/* DATA_STRUCT_H */

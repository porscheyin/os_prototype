/* 
 * File: data_structure.c
 * Author: Yin Zhang
 * Date: September 11, 2013
 * Description: This file contains the implementation of
 * the common data structures which are used by process
 * management module.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "data_struct.h"

// Description: Create and initialize a linked list.
// Parameter: None.
// Return: The newly created linked list. If creation fails, NULL is returned.

List *list_create()
{
    List *list;
    if ((list = malloc(sizeof *list)) == NULL)
        return NULL;
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

// Description: Free a linked list.
// Parameter @list: The list to free.
// Parameter @free_data: The callback used to free data of the list.
// Return: None.

void list_free(List *list, FuncFreeData free_data)
{
    void *data;

    if (list == NULL)
        return;

    // Remove each element.
    while (!list_is_empty(list))
    {
        if (list_remove_next(list, NULL, (void **) &data)
                == 0 && free_data != NULL)
        {
            // Call a user-defined function to free dynamically allocated data.
            free_data(data);
        }
    }

    free(list);
}

// Description: Insert an new element after the given one using the data given into a list.
// Parameter @list: The list where the data will be inserted into.
// Parameter @element: The element of the list where the data should be inserted after.
// Parameter @data: The data will be inserted.
// Return: On success, 1 is returned.  On error, 0 is returned.

int list_insert_next(List *list, ListElement *element, const void *data)
{
    ListElement *new_element;

    if (list == NULL || data == NULL)
        return 0;

    if ((new_element = malloc(sizeof *new_element)) == NULL)
        return 0;

    // Insert the element into the list.
    new_element->data = (void *) data;

    if (element == NULL)
    {
        // Insert at the head of the list.
        if (list_is_empty(list))
            list->tail = new_element;
        new_element->next = list->head;
        list->head = new_element;
    }
    else
    {
        // Insert somewhere other than at the head.
        if (element->next == NULL)
            list->tail = new_element;

        new_element->next = element->next;
        element->next = new_element;
    }
    // Adjust the size of the list.
    list->size++;

    return 1;
}

// Description: Insert a new element to a list in certain order.
// Parameter @list: The list where the data will be inserted into.
// Parameter @compare: The callback used to compare data in the element.
// Parameter @order: The order when inserting data into the list. 1 indicates ascending, 0 means descending.
// Parameter @data: The data will be inserted.
// Return: On success, 1 is returned.  On error, 0 is returned.

int list_insert_orderly(List *list, FuncCompare compare, int order,
                        const void *data)
{
    assert(list && compare && data);
    assert(order == 1 || order == 0);

    int result;
    const void *cur_data;
    ListElement *prev_element;
    ListElement *cur_element;

    // If list is empty, just insert.
    if (list_is_empty(list))
    {
        list_insert_next(list, NULL, data);
        return 1;
    }
    else
    {
        prev_element = NULL;
        cur_element = list_head(list);
    }
    for (;;)
    {
        // Compare current data to the new data.
        cur_data = list_data(cur_element);

        result = compare(data, cur_data);

        if (order == 1) // Insert in ascending order.
        {
            if (result < 0)
            {
                list_insert_next(list, prev_element, data);
                return 1;
            }
            else if (result >= 0)
            {
                if (list_next(cur_element) == NULL)
                {
                    list_insert_next(list, cur_element, data);
                    return 1;
                }
                else
                {
                    prev_element = cur_element;
                    cur_element = list_next(cur_element);
                    continue;
                }
            }
        }
        else if (order == 0) // Insert in descending order.
        {
            if (result >= 0)
            {
                list_insert_next(list, prev_element, data);
                return 1;
            }
            else if (result < 0)
            {
                if (list_next(cur_element) == NULL)
                {
                    list_insert_next(list, cur_element, data);
                    return 1;
                }
                else
                {
                    prev_element = cur_element;
                    cur_element = list_next(cur_element);
                    continue;
                }
            }
        }
    }
    return 0; // If reach here, there must be an error, return 0;
}

// Description: Remove an element after the given one from a list .
// Parameter @list: The list where the data will be removed from.
// Parameter @element: The element where the data will be removed after
// Parameter @data: Data returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int list_remove_next(List *list, ListElement *element, void **data)
{
    ListElement *old_element;

    assert(list && data);

    if (list_is_empty(list))
        return 0;

    if (element == NULL)
    {
        // Removal from the head.
        *data = list->head->data;
        old_element = list->head;
        list->head = list->head->next;
        if (list_size(list) == 1)
            list->tail = NULL;
    }
    else
    {
        // Remove from somewhere other than the head.
        if (element->next == NULL)
            return 0;
        *data = element->next->data;
        old_element = element->next;
        element->next = element->next->next;
        if (element->next == NULL)
            list->tail = element;
    }

    // Free the removed element.
    free(old_element);
    // Adjust the size of the list.
    list->size--;

    return 1;
}

// Description: Find an element in a list.
// Parameter @list: The list where the data will be found.
// Parameter @equal: The callback used to match data in the element.
// Parameter @data: The data will be found.
// Return: On success, the pointer of the element is returned.  On failure, NULL is returned.

ListElement *list_find_element(List *list, FuncMatch equal, const void *data)
{
    ListElement *element;

    assert(list && equal && data);

    // Iterate over elements in the list until the data is found.
    for (element = list->head; element; element = element->next)
    {
        if (equal(element->data, data))
        {
            return element;
        }
    }

    // Not found.
    return NULL;
}

// Description: Remove an element from a list.
// Parameter @list: The list where the data will be removed from.
// Parameter @element: The element where the data will be removed.
// Parameter @data: Data returned from the removed element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int list_remove_element(List *list, ListElement *element, void **data)
{
    ListElement *tmp;

    if (list == NULL || element == NULL)
    {
        return 0;
    }

    if (list_is_empty(list))
        return 0;

    if (list->head == element)
    {
        // Removal from the head.
        *data = list->head->data;
        list->head = list->head->next;
        if (list_size(list) == 1)
            list->tail = NULL;
    }
    else
    {

        // Search through the list to find the preceding element.
        tmp = list->head;

        while (tmp != NULL && tmp->next != element)
        {
            tmp = tmp->next;
        }

        if (tmp == NULL)
        {
            // Not found in list.
            return 0;
        }
        else
        {
            // tmp->next now points to element, so tmp is the preceding element.
            *data = element->data;
            tmp->next = tmp->next->next;
            if (tmp->next == NULL)
                list->tail = tmp;
        }
    }
    // Free the removed element.
    free(element);
    // Adjust the size of the list.
    list->size--;

    return 1;
}

// Description: Swap the data of two elements(Internal use).
// Return: None.

static void swap_data(ListElement *element1, ListElement *element2)
{
    assert(element1 && element2);
    void *temp;
    temp = element1->data;
    element1->data = element2->data;
    element2->data = temp;
}

// Description: Sort a list in a certain order.
// Parameter @list: The list to be sorted.
// Parameter @compare: The callback used to compare the data of the elements in the list.
// Parameter @order: 1 indicates ascending, 0 indicates descending.
// Return: None.

void list_sort(List *list, FuncCompare compare, int order)
{
    ListElement *head, *tail;
    ListElement *outer, *inner;

    assert(list && compare);
    assert(order == 1 || order == 0);

    if (list_size(list) < 2)
        return;

    head = list_head(list);
    tail = list_tail(list);

    for (outer = head; outer != tail; outer = outer->next)
    {
        for (inner = outer->next; inner; inner = inner->next)
        {
            if (order == 1) // Sort in ascending order.
            {
                if (compare(outer->data, inner->data) > 0)
                {
                    swap_data(outer, inner);
                }
            }
            else if (order == 0) // Sort in descending order.
            {
                if (compare(outer->data, inner->data) < 0)
                {
                    swap_data(outer, inner);
                }
            }
        }
    }
}

// Description: Check if a list contains an element with the given data.
// Parameter @list: The list is to check.
// Parameter @equal: The callback used to match the data of the element.
// Parameter @data: The data to be found.
// Return: 1 indicates contains. 0 indicates not contain.

int list_contains(List *list, FuncMatch equal, const void *data)
{
    assert(list && equal && data);

    if (list_find_element(list, equal, data))
        return 1;

    else
        return 0;
}

// Description: Get the size of a list.
// Parameter @list: The list to get size of.
// Return: The size of the list.

size_t list_size(List * list)
{
    assert(list);

    return list->size;
}

// Description: Check if a list is empty.
// Parameter @list: The list to check.
// Return: 1 indicates empty, 0 indicates not empty.

int list_is_empty(List *list)
{
    assert(list);

    return list->size == 0;
}

// Description: Get the head of a list.
// Parameter @list: The list to get head of.
// Return: The head of the list.

ListElement * list_head(List * list)
{
    assert(list);

    return list->head;
}

// Description: Get the tail of a list.
// Parameter @list: The list to get tail of.
// Return: The tail of the list.

ListElement *list_tail(List *list)
{
    assert(list);

    return list->tail;
}

// Description: Check if an element list is the head of a list.
// Parameter @list: The list to check.
// Parameter @element: The element to check.
// Return: 1 indicates the element is the head, 0 indicates the element is the not head.

int list_is_head(List *list, ListElement *element)
{
    assert(list && element);

    return element == list->head ? 1 : 0;
}

// Description: Check if an element list is the tail of a list.
// Parameter @list: The list to check.
// Parameter @element: The element to check.
// Return: 1 indicates the element is the tail, 0 indicates the element is the not tail.

int list_is_tail(ListElement *element)
{
    assert(element);

    return element->next == NULL ? 1 : 0;
}

// Description: Get the data of an element.
// Parameter @list: The element to get data of.
// Return: The data of the element.

void *list_data(ListElement *element)
{
    assert(element);

    return element->data;
}

// Description: Get the next element of a list.
// Parameter @list: The list to get next element of.
// Return: The next element of the list.

ListElement *list_next(ListElement *element)
{
    assert(element);

    return element->next;
}

// Description: Enqueue an element to a queue.
// Parameter @queue: The queue to enqueue.
// Parameter @data: The data of the newly enqueued element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int queue_enqueue(Queue *queue, const void *data)
{
    return list_insert_next(queue, list_tail(queue), data);
}

// Description: Dequeue an element from a queue.
// Parameter @queue: The queue to dequeue.
// Parameter @data: The data returned from the dequeued element.
// Return: On success, 1 is returned.  On error, 0 is returned.

int queue_dequeue(Queue *queue, void **data)
{
    return list_remove_next(queue, NULL, data);
}

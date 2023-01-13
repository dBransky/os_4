#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
struct MallocMetaData
{
    size_t size;
    bool is_free;
    MallocMetaData *next;
    MallocMetaData *prev;
};
size_t META_DATA_SIZE = sizeof(MallocMetaData);
MallocMetaData *first_allocation = NULL;
void split_block(MallocMetaData *ptr, size_t diff)
{
    if (diff >= 128 + META_DATA_SIZE)
    {
        MallocMetaData *larger = NULL;
        MallocMetaData *temp = first_allocation;
        while (temp != NULL)
        {
            if (!larger && temp->size > diff - META_DATA_SIZE)
            {
                larger = temp;
            }

            temp = temp->next;
        }
        *(MallocMetaData *)ptr = {diff - META_DATA_SIZE, true, larger, NULL};
        if (larger)
            larger->prev = ptr;
        if (larger->prev)
        {
            ptr->prev = larger->prev;
            larger->prev->next = ptr;
        }
    }
}
void *smalloc(size_t size)
{
    MallocMetaData *ptr = NULL;
    if (size == 0 || size > pow(10, 8))
        return ptr;
    MallocMetaData *temp = first_allocation;
    MallocMetaData *larger = NULL;
    while (temp != NULL)
    {
        if (temp->is_free && temp->size >= size)
        {
            temp->is_free = false;
            split_block(temp + META_DATA_SIZE + size, temp->size - META_DATA_SIZE - size);
            return temp + META_DATA_SIZE;
        }
        else if (!larger && temp->size > size)
        {
            larger = temp;
        }

        temp = temp->next;
    }
    ptr = (MallocMetaData *)sbrk(size + META_DATA_SIZE);
    if (*(int *)ptr == -1)
        return ptr;
    *(MallocMetaData *)ptr = {size, false, larger, NULL};
    if (larger)
        larger->prev = ptr;
    if (larger->prev)
    {
        ptr->prev = larger->prev;
        larger->prev->next = ptr;
    }

    if (!first_allocation)
        first_allocation = (MallocMetaData *)ptr;
    return ptr + size;
}
void *scalloc(size_t num, size_t size)
{
    void *ptr = smalloc(size * num);
    if (ptr)
    {
        for (size_t i = 0; i < num; i++)
        {
            memset((int *)ptr, 0, size);
        }
    }
    return ptr;
}
void sfree(void *ptr)
{
    MallocMetaData *block = (MallocMetaData *)(ptr - META_DATA_SIZE);
    if (block)
        block->is_free = true;
}
void *srealloc(void *oldp, size_t size)
{
    void *ptr = NULL;
    if (size == 0 || size > pow(10, 8))
        return ptr;
    if (!oldp)
    {
        return smalloc(size);
    }
    MallocMetaData *current_block = (MallocMetaData *)(oldp - META_DATA_SIZE);
    if (size <= current_block->size)
    {
        void *new_block = smalloc(size);
        if (new_block)
        {
            memmove(new_block, oldp, size);
            sfree(oldp);
        }
    }
}
size_t _num_free_blocks()
{
    MallocMetaData *temp = first_allocation;
    int free_blocks = 0;
    while (temp)
    {
        if (temp->is_free)
            free_blocks++;
        temp = temp->next;
    }
    return free_blocks;
}
size_t _num_free_bytes()
{
    MallocMetaData *temp = first_allocation;
    int free_bytes = 0;
    while (temp)
    {
        if (temp->is_free)
            free_bytes += temp->size;
        temp = temp->next;
    }
    return free_bytes;
}
size_t _num_allocated_blocks()
{
    MallocMetaData *temp = first_allocation;
    int allocated_blocks = 0;
    while (temp)
    {
        allocated_blocks++;
        temp = temp->next;
    }
    return allocated_blocks;
}
size_t _num_allocated_bytes()
{
    MallocMetaData *temp = first_allocation;
    int allocated_bytes = 0;
    while (temp)
    {
        allocated_bytes += temp->size;
        temp = temp->next;
    }
    return allocated_bytes;
}
size_t _num_meta_data_bytes()
{
    return (int)_num_allocated_blocks * META_DATA_SIZE;
}
size_t _size_meta_data()
{
    return META_DATA_SIZE;
}

int main()
{
    int *test = (int *)scalloc(5, 1);
    std::cout << test[0] << std::endl;
    std::cout << test[1] << std::endl;
    exit(1);
}
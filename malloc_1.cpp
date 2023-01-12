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
MallocMetaData *first_allocation = NULL;
void *smalloc(size_t size)
{
    void *ptr = NULL;
    if (size == 0 || size > pow(10, 8))
        return ptr;
    MallocMetaData *temp = first_allocation;
    while (temp != NULL)
    {
        if (temp->is_free && temp->size >= size)
        {
            temp->is_free = false;
            return temp + sizeof(MallocMetaData);
        }
        temp = temp->next;
    }
    ptr = sbrk(size + sizeof(MallocMetaData));
    if (*(int *)ptr == -1)
        return ptr;
    *(MallocMetaData *)ptr = {size, false, NULL, temp};
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
            memset((int*)ptr,0,size);
        }
    }
    return ptr;
}
void sfree(void *ptr)
{
    MallocMetaData *block = (MallocMetaData *)(ptr - sizeof(MallocMetaData));
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
    MallocMetaData *current_block = (MallocMetaData *)(oldp - sizeof(MallocMetaData));
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
    return (int)_num_allocated_blocks * sizeof(MallocMetaData);
}
size_t _size_meta_data()
{
    return sizeof(MallocMetaData);
}

int main()
{
    int *test = (int *)scalloc(5, 1);
    std::cout << test[0] << std::endl;
    std::cout << test[1] << std::endl;
    exit(1);
}
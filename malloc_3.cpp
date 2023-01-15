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
void remove_block(MallocMetaData *ptr)
{
    if (ptr->prev)
        ptr->prev->next = ptr->next;
    if (ptr->next)
        ptr->next->prev = ptr->prev;
}
void insert_block(MallocMetaData *ptr)
{
    MallocMetaData *larger = NULL;
    MallocMetaData *temp = first_allocation;
    while (temp != NULL)
    {
        if (!larger && temp->size > ptr->size)
        {
            larger = temp;
        }

        temp = temp->next;
    }
    ptr->next = larger;
    ptr->prev = larger->prev;
    if (larger)
    {
        if (larger->prev)
            larger->prev->next = ptr;
        larger->prev = ptr;
    }
}
void split_block(MallocMetaData *ptr, size_t diff)
{
    if (diff >= 128 + META_DATA_SIZE)
    {
    }
}
void merge_free_block(MallocMetaData *ptr)
{
    MallocMetaData *prev = NULL;
    MallocMetaData *next = NULL;
    MallocMetaData *temp = first_allocation;
    while (temp)
    {
        if (temp + META_DATA_SIZE + temp->size == ptr)
            prev = temp;
        if (ptr + META_DATA_SIZE + ptr->size == temp)
            next = temp;
    }
    if (next)
    {
        if (next->is_free)
            ptr->next =
                ptr->size += META_DATA_SIZE + next->size;
    }
    if (prev)
    {
        if (prev->is_free)
    }
}
void *smalloc(size_t size)
{
    MallocMetaData *ptr = NULL;
    if (size == 0 || size > pow(10, 8))
        return ptr;
    MallocMetaData *temp = first_allocation;
    MallocMetaData *last = NULL;
    while (temp)
    {
        if (temp->is_free && temp->size >= size)
        {
            temp->is_free = false;
            return (void *)((size_t)temp + sizeof(MallocMetaData));
        }
        last = temp;
        temp = temp->next;
    }
    ptr = (MallocMetaData *)sbrk(size + sizeof(MallocMetaData));
    if (*(int *)ptr == -1)
        return ptr;
    *ptr = {size, false, NULL, last};
    if (last)
        last->next = ptr;
    if (!first_allocation)
        first_allocation = (MallocMetaData *)ptr;
    return (void *)((size_t)ptr + (size_t)sizeof(MallocMetaData));
}
void *scalloc(size_t num, size_t size)
{
    void *ptr = smalloc(size * num);
    if (ptr)
    {
        memset(ptr, 0, size * num);
    }
    return ptr;
}
void sfree(void *ptr)
{
    MallocMetaData *block = (MallocMetaData *)((size_t)ptr - sizeof(MallocMetaData));
    if (block)
    {
        block->is_free = true;
    }
}
void *srealloc(void *oldp, size_t size)
{
    if (size == 0 || size > pow(10, 8))
        return NULL;
    if (!oldp)
    {
        return smalloc(size);
    }
    MallocMetaData *ptr = (MallocMetaData *)((size_t)oldp - sizeof(MallocMetaData));
    printf("given size %d\n",(int)size);
    printf("ptr size %d\n",(int)(ptr->size));
    if(ptr)
    {
        if (size >= ptr->size)
        {
            void *new_block = smalloc(size);
            if (new_block)
            {
                memmove(new_block, oldp, size);
                sfree(oldp);
                return new_block;
            }
        }
    }
    return (void *)((size_t)ptr + sizeof(MallocMetaData));
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
size_t _size_meta_data()
{
    return sizeof(MallocMetaData);
}
size_t _num_meta_data_bytes()
{
    return _num_allocated_blocks() * _size_meta_data();
}
// int main()
// {
//     int *test = (int *)scalloc(5, 1);
//     std::cout << test[0] << std::endl;
//     std::cout << test[1] << std::endl;
//     exit(1);
// }

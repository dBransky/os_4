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
//     void *base = sbrk(0);
//     char *a = (char *)smalloc(1);
//     void *after = sbrk(0);

//     char *b = (char *)smalloc(10);
//     after = sbrk(0);

//     _num_allocated_blocks();
//     sfree(a);
//     sfree(b);
// }
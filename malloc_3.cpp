#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <climits>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <vector>
#define assertm(exp, msg) assert(((void)msg, exp))
struct MallocMetaData
{
    size_t size;
    bool is_free;
    MallocMetaData *next;
    MallocMetaData *prev;
    bool is_mmapped;
    int32_t cookie;
};
bool CompareBlock(MallocMetaData *block1, MallocMetaData *block2)
{
    if (block1->size == block2->size)
        return (size_t)block1 < (size_t)block2;
    return block1->size < block2->size;
}
size_t META_DATA_SIZE = sizeof(MallocMetaData);
size_t MAX_SIZE = pow(10, 8);
int COOKIE = rand() + (rand() % 2) * INT32_MIN;
MallocMetaData *first_allocation = NULL;
MallocMetaData *wilderness_block = NULL;
std::vector<MallocMetaData *> blocks;
MallocMetaData *first_allocation_map = NULL;
void check_cookie_valid(MallocMetaData *ptr)
{
    if (ptr)
    {
        if (ptr->cookie != COOKIE)
            exit(0xdeadbeef);
    }
}
void remove_block(MallocMetaData *ptr)
{
    for (std::vector<MallocMetaData *>::iterator iter = blocks.begin(); iter != blocks.end(); ++iter)
    {
        if (*iter == ptr)
        {
            blocks.erase(iter);
            break;
        }
    }
    std::sort(blocks.begin(), blocks.end(), CompareBlock);
}
void insert_block(MallocMetaData *ptr)
{
    blocks.insert(blocks.begin(), ptr);
    std::sort(blocks.begin(), blocks.end(), CompareBlock);
}
void insert_block_map(MallocMetaData *ptr)
{
    if (!first_allocation_map)
    {
        first_allocation_map = ptr;
        return;
    }
    MallocMetaData *temp = first_allocation_map;
    while (temp->next)
    {
        temp = temp->next;
    }
    temp->next = ptr;
    ptr->prev = temp;
}
void remove_block_map(MallocMetaData *ptr)
{
    if (ptr == first_allocation_map)
        first_allocation_map = ptr->next;
    if (ptr->prev)
        ptr->prev->next = ptr->next;
    if (ptr->next)
        ptr->next->prev = ptr->prev;
}
void split_block(MallocMetaData *ptr, size_t new_size)
{
    if (ptr->size - new_size >= 128 + META_DATA_SIZE)
    {
        remove_block(ptr);
        MallocMetaData *new_block = (MallocMetaData *)((size_t)ptr + new_size + META_DATA_SIZE);
        new_block->is_free = true;
        new_block->size = ptr->size - new_size - META_DATA_SIZE;
        ptr->size = new_size;
        insert_block(new_block);
        insert_block(ptr);
    }
}
MallocMetaData *merge_higher(MallocMetaData *ptr)
{
    MallocMetaData *next = NULL;
    for (MallocMetaData *temp : blocks)
    {
        if ((void *)((size_t)ptr + META_DATA_SIZE + ptr->size) == temp)
            next = temp;
        temp = temp->next;
    }
    check_cookie_valid(ptr);
    check_cookie_valid(next);
    if (next)
    {
        if (next->is_free)
        {
            remove_block(next);
            remove_block(ptr);
            ptr->size += META_DATA_SIZE + next->size;
            if (wilderness_block == next)
                wilderness_block = ptr;
            insert_block(ptr);
        }
    }
    return ptr;
}
MallocMetaData *merge_lower(MallocMetaData *ptr)
{
    MallocMetaData *prev = NULL;
    for (MallocMetaData *temp : blocks)
    {
        if ((void *)((size_t)temp + META_DATA_SIZE + temp->size) == ptr)
            prev = temp;
        temp = temp->next;
    }
    check_cookie_valid(ptr);
    check_cookie_valid(prev);
    if (prev)
    {
        if (prev->is_free)
        {
            remove_block(prev);
            remove_block(ptr);
            prev->size += META_DATA_SIZE + ptr->size;
            insert_block(prev);
            if (ptr == wilderness_block)
                wilderness_block = prev;
            return prev;
        }
    }
    return ptr;
}
MallocMetaData *get_adj_lower(MallocMetaData *ptr)
{
    MallocMetaData *prev = NULL;
    for (MallocMetaData *temp : blocks)
    {
        if (temp + META_DATA_SIZE + temp->size == ptr)
            prev = temp;
        temp += META_DATA_SIZE + temp->size;
    }
    return prev;
}
MallocMetaData *get_adj_higher(MallocMetaData *ptr)
{

    MallocMetaData *next = NULL;
    for (MallocMetaData *temp : blocks)
    {
        if (ptr + META_DATA_SIZE + ptr->size == temp)
            next = temp;
        temp += META_DATA_SIZE + temp->size;
    }
    return next;
}
void *memory_map(size_t size)
{
    MallocMetaData *ptr = (MallocMetaData *)mmap(NULL, size + META_DATA_SIZE, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    perror("");
    *ptr = {size, false, NULL, NULL, true, COOKIE};
    insert_block_map(ptr);
    return (void *)((size_t)ptr + META_DATA_SIZE);
}

void *smalloc(size_t size)
{
    MallocMetaData *ptr = NULL;
    if (size == 0 || size > MAX_SIZE)
    {
        return ptr;
    }
    if (size >= 128 * 1024)
    {
        return memory_map(size);
    }
    for (MallocMetaData *temp : blocks)
    {
        if (temp->is_free && temp->size >= size)
        {
            check_cookie_valid(temp);
            split_block(temp, size);
            temp->is_free = false;
            return (void *)((size_t)temp + sizeof(MallocMetaData));
        }
        temp = temp->next;
    }
    check_cookie_valid(wilderness_block);
    if (wilderness_block)
    {
        if (wilderness_block->is_free)
        {
            ptr = wilderness_block;
            sbrk(size - ptr->size);
            remove_block(wilderness_block);
        }
        else
            ptr = (MallocMetaData *)sbrk(size + sizeof(MallocMetaData));
    }
    else
    {
        ptr = (MallocMetaData *)sbrk(size + sizeof(MallocMetaData));
        wilderness_block = ptr;
    }
    if ((size_t)ptr == (size_t)-1)
    {
        return ptr;
    }
    *ptr = {size, false, NULL, NULL, false, COOKIE};
    insert_block(ptr);
    wilderness_block = ptr;
    if (!first_allocation)
        first_allocation = ptr;
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
        if (block->is_mmapped)
        {
            remove_block_map(block);
            munmap(block, block->size);
        }
        else
        {
            block->is_free = true;
            merge_higher(block);
            merge_lower(block);
        }
    }
}
void *srealloc(void *oldp, size_t size)
{
    if (size == 0 || size > MAX_SIZE)
    {
        return NULL;
    }
    if (!oldp)
    {
        return smalloc(size);
    }
    MallocMetaData *ptr = (MallocMetaData *)((size_t)oldp - sizeof(MallocMetaData));
    check_cookie_valid(ptr);
    if (ptr->is_mmapped)
    {
        MallocMetaData *new_block = (MallocMetaData *)((size_t)(memory_map(size)) - (size_t)META_DATA_SIZE);
        check_cookie_valid(new_block);
        memmove(new_block, ptr, size);
        sfree(ptr);
        return (void *)((size_t)new_block + sizeof(MallocMetaData));
    }
    printf("given size %d\n", (int)size);
    printf("ptr size %d\n", (int)(ptr->size));
    if (ptr)
    {
        if (size >= ptr->size)
        {
            if (wilderness_block == ptr)
            {
                ptr = merge_lower(ptr);
                wilderness_block = ptr;
                wilderness_block->size += size - wilderness_block->size;
                sbrk(size - wilderness_block->size);
                split_block(ptr, size);
                return (void *)((size_t)ptr + sizeof(MallocMetaData));
            }
            MallocMetaData *lower = get_adj_lower(ptr);
            if (lower)
            {
                check_cookie_valid(lower);
                if (lower->is_free && lower->size + META_DATA_SIZE + ptr->size >= size)
                {
                    ptr = merge_lower(ptr);
                    split_block(ptr, size);
                    return (void *)((size_t)ptr + sizeof(MallocMetaData));
                }
            }
            MallocMetaData *higher = get_adj_higher(ptr);
            if (higher)
            {
                check_cookie_valid(higher);
                if (higher->is_free && higher->size + META_DATA_SIZE + ptr->size >= size)
                {
                    ptr = merge_higher(ptr);
                    split_block(ptr, size);
                    return (void *)((size_t)ptr + sizeof(MallocMetaData));
                }
            }
            if (higher && lower)
            {
                if (higher->is_free && lower->is_free && (higher->size + lower->size + 2 * META_DATA_SIZE > size))
                {
                    ptr = merge_higher(ptr);
                    ptr = merge_lower(ptr);
                    split_block(ptr, size);
                    return (void *)((size_t)ptr + sizeof(MallocMetaData));
                }
            }
            if (higher == wilderness_block)
            {
                if (higher->is_free)
                {
                    ptr = merge_higher(ptr);
                    ptr = merge_lower(ptr);
                    wilderness_block = ptr;
                    sbrk(size - ptr->size);
                    ptr->size += (size - ptr->size);
                    split_block(ptr, size);
                    return (void *)((size_t)ptr + sizeof(MallocMetaData));
                }
            }
            void *new_block = smalloc(size);
            if (new_block)
            {
                memmove(new_block, oldp, size);
                sfree(oldp);
                return new_block;
            }
        }
    }
    if (!ptr->is_mmapped)
        split_block(ptr, size);
    return (void *)((size_t)ptr + sizeof(MallocMetaData));
}
size_t _num_free_blocks()
{
    int free_blocks = 0;
    for (MallocMetaData *temp : blocks)
    {
        if (temp->is_free)
            free_blocks++;
        temp = temp->next;
    }
    return free_blocks;
}
size_t _num_free_bytes()
{
    int free_bytes = 0;
    for (MallocMetaData *temp : blocks)
    {
        if (temp->is_free)
            free_bytes += temp->size;
        temp = temp->next;
    }
    return free_bytes;
}
size_t _num_allocated_blocks()
{
    int allocated_blocks = 0;
    for (MallocMetaData *temp : blocks)
    {
        allocated_blocks++;
        temp = temp->next;
    }
    MallocMetaData *temp = first_allocation_map;
    while (temp)
    {
        allocated_blocks++;
        temp = temp->next;
    }
    return allocated_blocks;
}
size_t _num_allocated_bytes()
{
    int allocated_bytes = 0;
    for (MallocMetaData *temp : blocks)
    {
        allocated_bytes += temp->size;
        temp = temp->next;
    }
    MallocMetaData *temp = first_allocation_map;
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
#define MAX_ALLOCATION_SIZE (1e8)
#define MMAP_THRESHOLD (128 * 1024)
#define MIN_SPLIT_SIZE (128)
#define verify_blocks(allocated_blocks, allocated_bytes, free_blocks, free_bytes)           \
    do                                                                                      \
    {                                                                                       \
        printf("values are\n");                                                             \
        printf("%d==%d\n", _num_allocated_blocks(), allocated_blocks);                      \
        printf("%d==%d\n", _num_allocated_bytes(), (allocated_bytes));                      \
        printf("%d==%d\n", _num_free_bytes(), (free_bytes));                                \
        printf("%d==%d\n", _num_free_blocks(), free_blocks);                                \
        printf("%d==%d\n", _num_meta_data_bytes(), (_size_meta_data() * allocated_blocks)); \
    } while (0)
int main()
{
    verify_blocks(0, 0, 0, 0);
    void *base = sbrk(0);
    char *a = (char *)smalloc(16 + MIN_SPLIT_SIZE + _size_meta_data() - 8);
    // REQUIRE(a != nullptr);
    verify_blocks(1, 16 + MIN_SPLIT_SIZE + _size_meta_data() - 8, 0, 0);
    // verify_size(base);

    sfree(a);
    verify_blocks(1, 16 + MIN_SPLIT_SIZE + _size_meta_data() - 8, 1, 16 + MIN_SPLIT_SIZE + _size_meta_data() - 8);
    // verify_size(base);

    char *b = (char *)smalloc(16);
    // REQUIRE(b != nullptr);
    // REQUIRE(b == a);
    verify_blocks(1, 16 + MIN_SPLIT_SIZE + _size_meta_data() - 8, 0, 0);
    // verify_size(base);

    sfree(b);
    verify_blocks(1, 16 + MIN_SPLIT_SIZE + _size_meta_data() - 8, 1, 16 + MIN_SPLIT_SIZE + _size_meta_data() - 8);
    // verify_size(base);
}
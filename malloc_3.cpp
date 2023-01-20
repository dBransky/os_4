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
size_t META_DATA_SIZE = sizeof(MallocMetaData);
size_t MAX_SIZE = pow(10, 8);
int COOKIE = rand() + (rand() % 2) * INT32_MIN;
MallocMetaData *first_allocation = NULL;
MallocMetaData *smallest_allocation = NULL;
MallocMetaData *first_allocation_map = NULL;
MallocMetaData *wilderness_block = NULL;
bool list_valid()
{
    MallocMetaData *temp = smallest_allocation;
    if(!temp)
        return true;
    MallocMetaData *next = temp->next;
    while (temp && next)
    {
        if (temp == next)
        {
            return false;
        }
        temp = next;
        next = next->next;
    }
    return true;
}
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
    if (ptr->prev)
        ptr->prev->next = ptr->next;
    if (ptr->next)
        ptr->next->prev = ptr->prev;
    if (ptr == smallest_allocation)
        smallest_allocation = ptr->next;
}
void insert_block(MallocMetaData *ptr)
{
    MallocMetaData *larger = NULL;
    MallocMetaData *last = NULL;
    MallocMetaData *temp = smallest_allocation;
    while (temp)
    {
        check_cookie_valid(temp);
        if (!larger && temp->size > ptr->size)
        {
            larger = temp;
        }
        last = temp;
        temp = temp->next;
    }
    check_cookie_valid(ptr);
    check_cookie_valid(temp);
    ptr->next = larger;
    if (larger)
    {
        ptr->prev = larger->prev;
        if (larger->prev)
            larger->prev->next = ptr;
        larger->prev = ptr;
    }
    if (!last)
    {
        first_allocation = ptr;
        smallest_allocation = ptr;
    }
    else
    {
        last->next = ptr;
        ptr->prev = last;
    }
}
void insert_block_map(MallocMetaData *ptr)
{
    if (!first_allocation_map)
    {
        first_allocation_map = ptr;
        assert(list_valid());
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
    MallocMetaData *temp = smallest_allocation;
    while (temp)
    {
        if ((void *)((size_t)ptr + META_DATA_SIZE + temp->size) == temp)
            next = temp;
        temp = temp->next;
    }
    check_cookie_valid(temp);
    check_cookie_valid(ptr);
    check_cookie_valid(next);
    if (next)
    {
        if (next->is_free)
        {
            remove_block(next);
            remove_block(ptr);
            ptr->size += META_DATA_SIZE + next->size;
            insert_block(ptr);
        }
    }
    assert(list_valid());
    return ptr;
}
MallocMetaData *merge_lower(MallocMetaData *ptr)
{
    MallocMetaData *prev = NULL;
    MallocMetaData *temp = smallest_allocation;
    while (temp)
    {
        if ((void *)((size_t)temp + META_DATA_SIZE + temp->size) == ptr)
            prev = temp;
        temp = temp->next;
    }
    check_cookie_valid(temp);
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
            assert(list_valid());
            return prev;
        }
    }
    assert(list_valid());
    return ptr;
}
MallocMetaData *get_adj_lower(MallocMetaData *ptr)
{
    MallocMetaData *prev = NULL;
    MallocMetaData *temp = smallest_allocation;
    while (temp)
    {
        if (temp + META_DATA_SIZE + temp->size == ptr)
            prev = temp;
        temp += META_DATA_SIZE + temp->size;
    }
    assert(list_valid());
    return prev;
}
MallocMetaData *get_adj_higher(MallocMetaData *ptr)
{

    MallocMetaData *next = NULL;
    MallocMetaData *temp = smallest_allocation;
    while (temp)
    {
        if (ptr + META_DATA_SIZE + ptr->size == temp)
            next = temp;
        temp += META_DATA_SIZE + temp->size;
    }
    assert(list_valid());
    return next;
}
void *memory_map(size_t size)
{
    MallocMetaData *ptr = (MallocMetaData *)mmap(NULL, size + META_DATA_SIZE, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    perror("");
    *ptr = {size, false, NULL, NULL, true, COOKIE};
    insert_block_map(ptr);
    assert(list_valid());
    return (void *)((size_t)ptr + META_DATA_SIZE);
}

void *smalloc(size_t size)
{
    MallocMetaData *ptr = NULL;
    if (size == 0 || size > MAX_SIZE)
    {
        assert(list_valid());
        return ptr;
    }
    if (size > 128000)
    {
        assert(list_valid());
        return memory_map(size);
    }
    MallocMetaData *temp = smallest_allocation;
    check_cookie_valid(temp);
    while (temp)
    {
        if (temp->is_free && temp->size >= size)
        {
            check_cookie_valid(temp);
            split_block(temp, size);
            temp->is_free = false;
            assert(list_valid());
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
            sbrk(ptr->size - size);
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
    if (*(int *)ptr == -1)
    {
        assert(list_valid());
        return ptr;
    }
    *ptr = {size, false, NULL, NULL, false, COOKIE};
    insert_block(ptr);
    wilderness_block = ptr;
    assert(list_valid());
    return (void *)((size_t)ptr + (size_t)sizeof(MallocMetaData));
}
void *scalloc(size_t num, size_t size)
{
    void *ptr = smalloc(size * num);
    if (ptr)
    {
        memset(ptr, 0, size * num);
    }
    assert(list_valid());
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
        assert(list_valid());
        return NULL;
    }
    if (!oldp)
    {
        assert(list_valid());
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
        assert(list_valid());
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
                assert(list_valid());
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
                    assert(list_valid());
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
                    assert(list_valid());
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
                    assert(list_valid());
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
                    assert(list_valid());
                    return (void *)((size_t)ptr + sizeof(MallocMetaData));
                }
            }
            void *new_block = smalloc(size);
            if (new_block)
            {
                memmove(new_block, oldp, size);
                sfree(oldp);
                assert(list_valid());
                return new_block;
            }
        }
    }
    if (!ptr->is_mmapped)
        split_block(ptr, size);
    assert(list_valid());
    return (void *)((size_t)ptr + sizeof(MallocMetaData));
}
size_t _num_free_blocks()
{
    MallocMetaData *temp = smallest_allocation;
    int free_blocks = 0;
    while (temp)
    {
        if (temp->is_free)
            free_blocks++;
        temp = temp->next;
    }
    assert(list_valid());
    return free_blocks;
}
size_t _num_free_bytes()
{
    MallocMetaData *temp = smallest_allocation;
    int free_bytes = 0;
    while (temp)
    {
        if (temp->is_free)
            free_bytes += temp->size;
        temp = temp->next;
    }
    assert(list_valid());
    return free_bytes;
}
size_t _num_allocated_blocks()
{
    int allocated_blocks = 0;
    MallocMetaData *temp = smallest_allocation;
    while (temp)
    {
        allocated_blocks++;
        temp = temp->next;
    }
    temp = first_allocation_map;
    while (temp)
    {
        allocated_blocks++;
        temp = temp->next;
    }
    assert(list_valid());
    return allocated_blocks;
}
size_t _num_allocated_bytes()
{
    int allocated_bytes = 0;
    MallocMetaData *temp = smallest_allocation;
    while (temp)
    {
        allocated_bytes += temp->size;
        temp = temp->next;
    }
    temp = first_allocation_map;
    while (temp)
    {
        allocated_bytes += temp->size;
        temp = temp->next;
    }
    assert(list_valid());
    return allocated_bytes;
}
size_t _size_meta_data()
{
    assert(list_valid());
    return sizeof(MallocMetaData);
}
size_t _num_meta_data_bytes()
{
    assert(list_valid());
    return _num_allocated_blocks() * _size_meta_data();
}
#define verify_blocks(allocated_blocks, allocated_bytes, free_blocks, free_bytes)           \
    do                                                                                      \
    {                                                                                       \
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
    char *a = (char *)smalloc(10);
    // REQUIRE(a != nullptr);
    char *b = (char *)smalloc(10);
    // REQUIRE(b != nullptr);
    char *c = (char *)smalloc(10);
    // REQUIRE(c != nullptr);

    verify_blocks(3, 10 * 3, 0, 0);
    // verify_size(base);

    sfree(a);
    verify_blocks(3, 10 * 3, 1, 10);
    // verify_size(base);
    sfree(b);
    verify_blocks(2, 10 * 3 + _size_meta_data(), 1, 10 * 2 + _size_meta_data());
    // verify_size(base);
    sfree(c);
    verify_blocks(1, 10 * 3 + _size_meta_data() * 2, 1, 10 * 3 + _size_meta_data() * 2);
    // verify_size(base);

    char *new_a = (char *)smalloc(10);
    // REQUIRE(a == new_a);
    char *new_b = (char *)smalloc(10);
    // REQUIRE(b != new_b);
    char *new_c = (char *)smalloc(10);
    // REQUIRE(c != new_c);

    verify_blocks(3, 10 * 5 + _size_meta_data() * 2, 0, 0);
    // verify_size(base);

    sfree(new_a);
    verify_blocks(3, 10 * 5 + _size_meta_data() * 2, 1, 10 * 3 + _size_meta_data() * 2);
    // verify_size(base);
    sfree(new_b);
    verify_blocks(2, 10 * 5 + _size_meta_data() * 3, 1, 10 * 4 + _size_meta_data() * 3);
    // verify_size(base);
    sfree(new_c);
    verify_blocks(1, 10 * 5 + _size_meta_data() * 4, 1, 10 * 5 + _size_meta_data() * 4);
    // verify_size(base);
}
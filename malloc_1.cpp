#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
void *smalloc(size_t size)
{
    void *ptr = NULL;
    if (size == 0 || size > pow(10, 8))
        return ptr;
    ptr = sbrk(size);
    if (*(int *)ptr == -1)
        return ptr;
    return (ptr);
}
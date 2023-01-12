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
    return ptr+size;
}
int main(){
    int* test=(int*)smalloc(5);
    test[0]=1;
    test[1]=2;
    std::cout<<test[0]<<std::endl;
    std::cout<<test[1]<<std::endl;
    exit(1);

}
#include "syscall.h"

void forkFn()
{
    int a= 10 + 10;
}

int main()
{
    //typedef void (*VoidFunctionPtr)(void *arg); 
    //Fork(forkFn);
    Fork();

}
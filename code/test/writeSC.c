#include "syscall.h"
int main()
{
    char *buffer = "This is write system call";
    int size = Write(buffer, 13, 0);
}

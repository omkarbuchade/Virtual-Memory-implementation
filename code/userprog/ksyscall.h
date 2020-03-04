/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"

//method to call when Halt system call is hit
void SysHalt()
{
  kernel->interrupt->Halt();
} 

//method to call when add system call is hit
int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

//method to call when write system call is hit
int SysWrite(char *buffer, int size, OpenFileId id)
{
    std::cout<<"********** Write system call invoked ********** \n";
    std::cout<<"Value of the buffer to write is: "<<buffer;
    return size;
}

//method to call when read system call is hit
int SysRead(char *buffer, int size, OpenFileId id)
{
    std::cout<<"********** Read system call invoked ********** \n";
    std::cout<<"This is read buffer: ";
    for (int i = 0; i < size; i++)
    {
        std::cout<<buffer[i];
    }
    std::cout<<"\n";
    return size;
}

void SimpleThread1(int a)
{
        DEBUG(dbgSys, "Thread forked ");
}

//method to call when fork system call is hit
SpaceId SysFork()
{   
    std::cout<<"********** Fork system call invoked ********** \n";
    Thread *th = new Thread ("ForkSysCall");
    th->space = kernel->currentThread->space;
    th->Fork((VoidFunctionPtr) SimpleThread1, (void *) 1);

    SpaceId id = SpaceId(th);
    return (id);
}   

//method to call when exit system call is hit
void SysExit(int status)
{
    std::cout<<"********** Exit system call invoked ********** \n";
    std::cout<<"Thread id: "<<kernel->currentThread->getId()<<" completed \n";
    std::cout<<"--------------------------------------------------------------------\n";

    delete kernel->currentThread->space;
    kernel->currentThread->Finish();
}

void execMethod(void *file_exec)
{
    AddrSpace *space = new AddrSpace;
    ASSERT(space != (AddrSpace *)NULL);
    if (space->Load((char*)file_exec)) {  // load the program into the space
        space->Execute();         // run the program
    }
    ASSERTNOTREACHED();
}

//method to call when exec system call is hit
SpaceId SysExec(char* file_exec)
{
    std::cout<<"********** Exec system call invoked ********** \n";
    Thread *th = new Thread("Exec Thread ");
    th->Fork((VoidFunctionPtr) execMethod, (void *) file_exec);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */

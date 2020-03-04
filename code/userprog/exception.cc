// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include <cstdlib>

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

int count=0;
void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

    switch (which) {

    case PageFaultException:
    {
        count++;
        std::cout <<"Page fault exception no: "<<count<<"\n";
        unsigned int vpn;
        int vaddr = kernel->machine->ReadRegister(BadVAddrReg);
        vpn =  (unsigned) vaddr / PageSize;
        char *buff = new char[PageSize];
        
        kernel->swapFile->ReadAt(buff, PageSize, kernel->currentThread->space->maptoSwap[vpn]*PageSize);        

        int physicalPageNo = kernel->bitmap->FindAndSet(); 
        if (physicalPageNo != -1) //check if there is availability in main memory
        {
            //std::cout<<"memory avail \n";
            kernel->currentThread->space->setAttr(TRUE, physicalPageNo, vpn);            
            kernel->Phypage_ThreadMap.insert(pair<int, Thread*>(physicalPageNo, kernel->currentThread));
            

            //bzero (&kernel->machine->mainMemory[physicalPageNo *PageSize], PageSize);
            bcopy (buff, &kernel->machine->mainMemory[physicalPageNo *PageSize], PageSize);
            std::cout<<"page "<< count<<" copied to main memory \n";
        }
 
        else // main memory full. Choose page to swap from main memory (random page replacement algorithm)
        {
            std::cout<<"********** Main memory full. Swapping a page from main memory to swap file ********** \n";
            int randNo = rand()%128;
            char *buff1 = new char[PageSize];

            bcopy (&kernel->machine->mainMemory[randNo *PageSize], buff1, PageSize);
            kernel->bitmap->Clear(randNo);

            Thread *swapThread = kernel->Phypage_ThreadMap[randNo];
            
            int vpn_rep = swapThread->space->getVPN(randNo);
            if(vpn_rep!=-1)
            {
                int swapIndex = swapThread->space->maptoSwap[vpn_rep];
                kernel->swapFile->WriteAt(buff1, PageSize, swapIndex*PageSize);
                kernel->Phypage_ThreadMap.erase(randNo);

                int physicalPageNo = kernel->bitmap->FindAndSet(); 
                if (physicalPageNo != -1) //check if there is availability in main memory
                {  
                    kernel->currentThread->space->setAttr(TRUE, physicalPageNo, vpn);            
                    kernel->Phypage_ThreadMap.insert(pair<int, Thread*>(physicalPageNo, kernel->currentThread));
                    bcopy (buff, &kernel->machine->mainMemory[physicalPageNo *PageSize], PageSize);
                    std::cout<<"page replaced \n";
                }
            }
            delete buff1;
        }
    delete buff;

    return;
    ASSERTNOTREACHED();
	break;
    }
    case SyscallException:
      switch(type) {
      case SC_Halt:
	DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

	SysHalt();

	ASSERTNOTREACHED();
	break;

      case SC_Add:
	DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
	
	/* Process SysAdd Systemcall*/
	int result;
	result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));

	DEBUG(dbgSys, "Add returning with " << result << "\n");
	/* Prepare Result */
	kernel->machine->WriteRegister(2, (int)result);
	
	/* Modify return point */
	{
	  /* set previous programm counter (debugging only)*/
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	  /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	  
	  /* set next programm counter for brach execution */
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}

	return;
	
	ASSERTNOTREACHED();

	break;

    case SC_Write:
    {
        DEBUG(dbgSys, "Write SystemCall:  value of register 4 is: " << kernel->machine->ReadRegister(4) << " value of register 5 is: " << kernel->machine->ReadRegister(5) << "\n");
        int address = (int)kernel->machine->ReadRegister(4);				 
        int size = (int)kernel->machine->ReadRegister(5);
        char buffer[100];

        for (int i = 0; i < size; i++) {
            kernel->machine->ReadMem(address++, 1, (int*)&buffer[i]);
        }

        SpaceId result = SysWrite((char*)buffer, (int)kernel->machine->ReadRegister(5),kernel->machine->ReadRegister(5));
        kernel->machine->WriteRegister(2, (int)result);
        
        /* Modify return point */
        {
            /* set previous programm counter (debugging only)*/
            kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

            /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
            kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

            /* set next programm counter for brach execution */
            kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
	}
    break;

    case SC_Exit:
		{   
            std::cout<<"\n--------------------------------------------------------------------\n";
            std::cout<<"Total memory references: "<<count+kernel->hit<<"\n";
            std::cout<<"Total page faults: "<<count<<"\n";
            std::cout<<"Total page hit: "<<kernel->hit<<"\n";
            std::cout<<"register 4: "<<(int)kernel->machine->ReadRegister(4)<<"\n";
			SysExit((int)kernel->machine->ReadRegister(4));

    
        return;
        ASSERTNOTREACHED();
		}
		break;

    case SC_Fork:
    {
        SpaceId id= SysFork();
        kernel->machine->WriteRegister(2, int(id));

                /* Modify return point */
        {
            /* set previous programm counter (debugging only)*/
            kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

            /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
            kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

            /* set next programm counter for brach execution */
            kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
    }
    break;

    case SC_Read:
    {
        char *name= new char[10];
        char buffer[5];

        int address= (int) kernel->machine->ReadRegister(4);
        int size= (int) kernel->machine->ReadRegister(5);

        for (int i=0 ; i< size; i++ )
        {
            kernel->machine->ReadMem(address++,1,(int *)&buffer[i]);
        }

        SpaceId result = SysRead(buffer, size, 0);
        kernel->machine->WriteRegister(2, int(result));
          
                /* Modify return point */
        {
            /* set previous programm counter (debugging only)*/
            kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

            /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
            kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

            /* set next programm counter for brach execution */
            kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
    }
    break;


 case SC_Exec:
    {
        int address= (int) kernel->machine->ReadRegister(4);
        char buffer[15];

        for (int i=0 ; i< 15; i++ )
        {
            kernel->machine->ReadMem(address++,1,(int *)&buffer[i]);
        }

        SpaceId spaceID = SysExec((char*)buffer);
        kernel->machine->WriteRegister(2, int(spaceID));
      
                /* Modify return point */
        {
            /* set previous programm counter (debugging only)*/
            kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

            /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
            kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

            /* set next programm counter for brach execution */
            kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
    }
    break;

    default:
	cerr << "Unexpected system call " << type << "\n";
	break;
    }
      break;
    default:
      cerr << "Unexpected user mode exception " << (int)which << "\n";
      break;
    }
    ASSERTNOTREACHED();
}



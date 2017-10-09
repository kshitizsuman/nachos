// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"
#include <fstream>
#include <string>

extern void ForkStartFunction(int dummy);

void
 BatchStartFunction(int dummy)
 {
    currentThread->Startup();
    DEBUG('b', "Now running %s thread\n", currentThread->getName());
    machine->Run();
 }
 

//----------------------------------------------------------------------
// StartUserProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartUserProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    ProcessAddrSpace *space;

    if (executable == NULL) {
		printf("Unable to open file %s\n", filename);
		return;
    }
    space = new ProcessAddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitUserCPURegisters();		// set the initial register values
    space->RestoreStateOnSwitch();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// RunBatchProcess 
// Run a batch of processes
//----------------------------------------------------------------------

void
RunBatchProcess(char *filename) {
    // Open the given executable
    /* OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }
    */

    std::ifstream infile(filename);
    std::string line;

    int tmp, priority, i, len;
    char testfile[256];

    /*// Compute the length of the file and then read that much from the file
    int filelength = executable->Length();
    char readbuffer[filelength];

    // Read the executable
    executable->ReadAt(readbuffer, (filelength-1), 0);
    DEBUG('b', "running batch jobs from %s of length %d\n", filename, filelength );*/

    ProcessAddrSpace *newspace;
    NachOSThread *newthread;

    IntStatus oldlevel = interrupt->SetLevel(IntOff);
    while (getline(infile, line)) {
        int pos = line.find(' ');
        if (pos >= 0) {
            len = pos;
        } else {
            len = line.length();
        }
        for(i = 0; i < len; ++i) {
            testfile[i] = line[i];
        }
        testfile[i]='\0';
        if (pos >= 0) {
            tmp = 0;
            ++ pos;
            while(line[pos] != '\n' && line[pos] >= '0' && line[pos] <= '9')  {
                tmp = tmp * 10;
                tmp += (line[pos] - '0');
                pos ++;
            }
            priority = tmp;
        } else {
            priority = 100;
        }
        printf("%s %d\n", testfile, priority);

        OpenFile *executable = fileSystem->Open(testfile);

        if (executable == NULL) {
			printf("Unable to open file %s\n", filename);
			return;
	    }
	    newspace = new ProcessAddrSpace(executable);

	    newthread = new NachOSThread(testfile);
	    newthread->space = newspace;
	
	    newspace->InitUserCPURegisters();		// set the initial register values
        newthread->SaveUserState();            // Duplicate the register set

        newthread->AllocateThreadStack(ForkStartFunction, 0);
        // Make it ready for a later context switch

        newthread->Schedule();
    }
    exitThreadArray[currentThread->GetPID()] = true;

    interrupt->SetLevel(oldlevel);

    currentThread->Exit(FALSE, 0);
   
    /*// Create threads and enque them
    int i=0, k=0;
    char name[100];
    char priority[10];


    while(readbuffer[i]!='\n') {
         name[k]=readbuffer[i];
        ++i; ++k;
     }
     name[k]='\0';
    k=0; ++i;
    DEBUG('b', "The scheduling algorithm is \"%s\"\n", name);
    // For the first thread obtain the priority of the thread and it's name, 
    // overwrite the current thread's address space with this program
    // and then go on to create new threads for the other processes
    
    while(i<filelength){
         if(readbuffer[i]=='\n') {
            name[k]='\0';
            k=0; ++i;
            while(readbuffer[i]!='\n') {
                priority[k]=readbuffer[i];
                ++i; ++k;
            }
            priority[k]='\0';
            k=0; ++i;

            // Creating a new thread and enquing it
            DEBUG('b', "%s %s\n", name, priority);

            programfile = fileSystem->Open(name);
            ASSERT(programfile != NULL);
            thread = new NachOSThread(name,100,true);
            thread->space = new ProcessAddrSpace(programfile);  // Duplicates the address space
            thread->space->InitUserCPURegisters();     // set the initial register values
            thread->SaveUserState();
            thread->AllocateThreadStack(BatchStartFunction, 0);   // Make it ready for a later context switch
            thread->Schedule();
            delete programfile;         // close file
        } 
        else {
            // This is the normal case when priority has been suplied
            if(readbuffer[i]!=' '){
                name[k]=readbuffer[i];
                ++i; ++k;
            } 
            else {
                name[k]='\0';
                k=0; ++i;
                while(readbuffer[i]!='\n') {
                    priority[k]=readbuffer[i];
                    ++i; ++k;
                }
                priority[k]='\0';
                k=0; ++i;

                // Creating a new thread and enquing it
                programfile = fileSystem->Open(name);
                ASSERT(programfile != NULL);
                thread = new NachOSThread(name, atoi(priority), true);
                thread->space = new ProcessAddrSpace(programfile);  // Duplicates the address space
                thread->space->InitUserCPURegisters();     // set the initial register values
                thread->SaveUserState();        // load page table register
                thread->AllocateThreadStack(BatchStartFunction, 0);   // Make it ready for a later context switch
                thread->Schedule();
                delete programfile;         // close file
            }
        }
    }
    currentThread->FinishThread();*/
}


// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}

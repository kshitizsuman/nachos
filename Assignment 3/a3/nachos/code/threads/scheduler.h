// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class NachOSscheduler {
  public:
    NachOSscheduler();			// Initialize list of ready threads 
    ~NachOSscheduler();			// De-allocate ready list

    void ThreadIsReadyToRun(NachOSThread* thread);	// NachOSThread can be dispatched.
    NachOSThread* FindNextThreadToRun();		// Dequeue first thread on the ready 
					// list, if any, and return thread.
    void Schedule(NachOSThread* nextThread);	// Cause nextThread to start running
    void Print();			// Print contents of ready list

    void Tail();			// Used by fork()

    void SetEmptyReadyQueueStartTime (int ticks);

    void UpdateThreadPriority (void);	// Used by the UNIX scheduler
   
  private:
    List *readyThreadList;  		// queue of threads that are ready to run,
				// but not running

    int empty_ready_queue_start_time;
};

#endif // SCHEDULER_H

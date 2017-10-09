// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// ProcessAddrSpace::ProcessAddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

ProcessAddrSpace::ProcessAddrSpace(OpenFile *executable)
{
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPagesInVM = divRoundUp(size, PageSize);
     
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    for (i = 0; i < numPagesInVM; i++) {
	NachOSpageTable[i].virtualPage = i;
	NachOSpageTable[i].physicalPage = -1;
	NachOSpageTable[i].valid = FALSE;
	NachOSpageTable[i].use = FALSE;
	NachOSpageTable[i].dirty = FALSE;
	NachOSpageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    NachOSpageTable[i].isShared = FALSE;
    }
    validpages = 0;


    numberofpagesshared = 0;
    
}

//----------------------------------------------------------------------
// ProcessAddrSpace::ProcessAddrSpace (ProcessAddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

ProcessAddrSpace::ProcessAddrSpace(ProcessAddrSpace *parentSpace)
{
    int temp;
    int *phypgnum;
    unsigned parentadd, childadd;
    numPagesInVM = parentSpace->GetNumPages();
    noffH = parentSpace->noffH;
    numberofpagesshared = parentSpace->numberofpagesshared;
    validpages = parentSpace->validpages;
    strcpy(filename, parentSpace->filename);
    unsigned i,j;
    numPagesAllocated += validpages-numberofpagesshared;
    ASSERT(numPagesAllocated <= NumPhysPages)
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    for (i = 0; i < numPagesInVM; i++) {
        if(parentPageTable[i].isShared){
            NachOSpageTable[i].physicalPage = parentPageTable[i].physicalPage;
        } else {
            if(parentPageTable[i].valid) {
                phypgnum = (int *)freepagelist->Remove();
                if(phypgnum == NULL) {
                    NachOSpageTable[i].physicalPage = nextpagetoallocate;
                    nextpagetoallocate++;   // Update the number of pages allocated
                } else {
                    NachOSpageTable[i].physicalPage = *phypgnum;
                }

            } else {
                NachOSpageTable[i].physicalPage = -1;
            }
        }
        NachOSpageTable[i].valid = parentPageTable[i].valid;
        NachOSpageTable[i].virtualPage = i;
        NachOSpageTable[i].use = parentPageTable[i].use;
        NachOSpageTable[i].dirty = parentPageTable[i].dirty;
        NachOSpageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
        NachOSpageTable[i].isShared = parentPageTable[i].isShared;
        
    }
    for (i=0; i<numPagesInVM; i++) {
        if(!NachOSpageTable[i].isShared && NachOSpageTable[i].valid){
            
            temp=parentPageTable[i].physicalPage * PageSize;
            
            parentadd = temp;
            
            temp = NachOSpageTable[i].physicalPage * PageSize;
            

            childadd = temp;
            
            temp = PageSize;

            for(j=0; j<temp;++j) {
                machine->mainMemory[childadd+j] = machine->mainMemory[parentadd+j];
            }
        }
    }

    
}

unsigned
ProcessAddrSpace::makesharedtable(int sharedSize, int *pagesCreated)
{
    unsigned originalPages = GetNumPages();
    int remain,*phypgnum;
    unsigned pagesshared = sharedSize / PageSize,i;
    remain=sharedSize % PageSize;
    if ( remain ) pagesshared =pagesshared +1;
    numberofpagesshared += pagesshared;
    TranslationEntry* origintable = GetPageTable();
    numPagesInVM = originalPages + pagesshared;
    *pagesCreated = pagesshared;
    numPagesAllocated = numPagesAllocated + pagesshared;
    ASSERT(numPagesAllocated <= NumPhysPages);
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    for (i = 0; i < originalPages; i++) {
        NachOSpageTable[i].virtualPage = i;
        NachOSpageTable[i].physicalPage = origintable[i].physicalPage;
        NachOSpageTable[i].valid = origintable[i].valid;
        NachOSpageTable[i].use = origintable[i].use;
        NachOSpageTable[i].dirty = origintable[i].dirty;
        NachOSpageTable[i].readOnly = origintable[i].readOnly; 
        NachOSpageTable[i].isShared = origintable[i].isShared;
    }
    for(i = originalPages; i <numPagesInVM; ++i) {
        NachOSpageTable[i].virtualPage = i;
        int *phypgnum = (int *)freepagelist->Remove();
        if(phypgnum == NULL) {
            NachOSpageTable[i].physicalPage = nextpagetoallocate;
            bzero(&machine->mainMemory[nextpagetoallocate*PageSize], PageSize);
            nextpagetoallocate++;   // Update the number of pages allocated
        } else {
            NachOSpageTable[i].physicalPage = *phypgnum;
        }

        NachOSpageTable[i].valid = TRUE;
        NachOSpageTable[i].use = FALSE;
        NachOSpageTable[i].dirty = FALSE;
        NachOSpageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
        NachOSpageTable[i].isShared = TRUE; // this is a shared region
    }
    validpages = validpages+pagesshared;
    machine->NachOSpageTable = NachOSpageTable;
    machine->NachOSpageTableSize = numPagesInVM * PageSize;
    delete origintable;
    return originalPages * PageSize;
}



//----------------------------------------------------------------------
// ProcessAddrSpace::~ProcessAddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

ProcessAddrSpace::~ProcessAddrSpace()
{
   delete NachOSpageTable;
}

//----------------------------------------------------------------------
// ProcessAddrSpace::InitUserCPURegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
ProcessAddrSpace::InitUserCPURegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPagesInVM * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPagesInVM * PageSize - 16);
}

//----------------------------------------------------------------------
// ProcessAddrSpace::SaveStateOnSwitch
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void ProcessAddrSpace::deletepage() {
    int i,*var,no_phy_page,num=0;
    for (i = 0; i < numPagesInVM; i++) {
        if(NachOSpageTable[i].valid) {     

            if(!NachOSpageTable[i].isShared){
                no_phy_page=NachOSpageTable[i].physicalPage;
                
                var = new int(no_phy_page);
                
                freepagelist->Append((void *)var);
                num=num+1;
            }    
        }
    }
    delete NachOSpageTable;
    

    numPagesAllocated = numPagesAllocated - num;

}




void ProcessAddrSpace::SaveStateOnSwitch() 
{}

//----------------------------------------------------------------------
// ProcessAddrSpace::RestoreStateOnSwitch
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void ProcessAddrSpace::RestoreStateOnSwitch() 
{
    machine->NachOSpageTable = NachOSpageTable;
    machine->NachOSpageTableSize = numPagesInVM;
}

unsigned
ProcessAddrSpace::GetNumPages()
{
   return numPagesInVM;
}

TranslationEntry*
ProcessAddrSpace::GetPageTable()
{
   return NachOSpageTable;
}


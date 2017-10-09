#include "syscall.h"
#define OUTER_BOUND 4
#define SIZE 100

int
main()
{
    int array[SIZE], i, k, sum, pid=system_call_GetPID()-1;
    unsigned start_time, end_time;
    
    for (k=0; k<OUTER_BOUND; k++) {
       for (i=0; i<SIZE; i++) sum += array[i];
       system_call_PrintInt(pid);
       system_call_PrintInt(pid);
    }
    system_call_PrintChar('\n');
    system_call_PrintString("Total sum: ");
    system_call_PrintInt(sum);
    system_call_PrintChar('\n');
    return 0;
}

#include "syscall.h"

#define NUM_ITER 200

int
main()
{
    int *array = (int*)system_call_ShmAllocate(2*sizeof(int));
    int x, i;

    array[0] = 0;
    array[1] = 100;

    x = system_call_Fork();
    if (x == 0) {
       array[0] = 20;
       for (i=0; i<NUM_ITER; i++) {
          array[0]++;
       }
    }
    else {
       array[1] = 200;
       for (i=0; i<NUM_ITER; i++) {
          array[1]++;
       }
       x=system_call_Join(x);
       system_call_PrintString("Array[0]=");
       system_call_PrintInt(array[0]);
       system_call_PrintChar('\n');
       system_call_PrintString("Array[1]=");
       system_call_PrintInt(array[1]);
       system_call_PrintChar('\n');
    }
    return 0;
}

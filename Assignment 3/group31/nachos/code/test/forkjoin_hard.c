#include "syscall.h"

int
main()
{
    int x, y=6;
    int sleep_start, sleep_end;

    system_call_PrintString("Parent PID: ");
    system_call_PrintInt(system_call_GetPID());
    system_call_PrintChar('\n');
    x = system_call_Fork();
    if (x == 0) {
       system_call_PrintString("Child PID: ");
       system_call_PrintInt(system_call_GetPID());
       system_call_PrintChar('\n');
       system_call_PrintString("Child's parent PID: ");
       system_call_PrintInt(system_call_GetPPID());
       system_call_PrintChar('\n');
       sleep_start = system_call_GetTime();
       system_call_Sleep(100);
       sleep_end = system_call_GetTime();
       system_call_PrintString("Child called sleep at time: ");
       system_call_PrintInt(sleep_start);
       system_call_PrintChar('\n');
       system_call_PrintString("Child returned from sleep at time: ");
       system_call_PrintInt(sleep_end);
       system_call_PrintChar('\n');
       y++;
       system_call_PrintString("Child y=");
       system_call_PrintInt(y);
       system_call_PrintChar('\n');
       x = system_call_Fork();
       system_call_Exec("../test/printtest");
       if (x == 0) {
          system_call_PrintString("Child PID: ");
          system_call_PrintInt(system_call_GetPID());
          system_call_PrintChar('\n');
          y++;
          system_call_PrintString("Child2 y=");
          system_call_PrintInt(y);
          system_call_PrintChar('\n');
          system_call_Exit(20);
       }
       else {
          system_call_PrintString("Parent after fork waiting for child: ");
          system_call_PrintInt(x);
          system_call_PrintChar('\n');
          system_call_PrintString("Parent2 join value: ");
          system_call_PrintInt(system_call_Join(x));
          system_call_PrintChar('\n');
          system_call_PrintString("Parent2 y=");
          system_call_PrintInt(y);
          system_call_PrintChar('\n');
          system_call_Exit(10);
       }
    }
    else {
       system_call_PrintString("Parent after fork waiting for child: ");
       system_call_PrintInt(x);
       system_call_PrintChar('\n');
       system_call_PrintString("Parent2 join value: ");
       system_call_PrintInt(system_call_Join(x));
       system_call_PrintChar('\n');
       system_call_PrintString("Parent y=");
       system_call_PrintInt(y);
       system_call_PrintChar('\n');
       system_call_Exit(1);
    }
    return 0;
}

#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>

static int ClockDriver(void *arg);
int P2_Spawn(char *name, int(*func)(void *), void *arg, int stackSize, int priority);
int P2_Wait(int *status);

int
P2_Startup(void *arg)
{
    P1_Semaphore      running;
    int               status;
    int               pid;
    int               clockPID;
    /*
     * Check kernel mode
     */
    // ...

    /*
     * Create clock device driver 
     */
    running = P1_SemCreate(0);
    clockPID = P1_Fork("Clock driver", ClockDriver, (void *) running, USLOSS_MIN_STACK, 2);
    if (clockPID == -1) {
        USLOSS_Console("Can't create clock driver\n");
    }
    /*
     * Wait for the clock driver to start.
     */
    P1_P(running);
    /*
     * Create the other device drivers.
     */
    // ...

    /* 
     * Create initial user-level process. You'll need to check error codes, etc. P2_Spawn
     * and P2_Wait are assumed to be the kernel-level functions that implement the Spawn and 
     * Wait system calls, respectively (you can't invoke a system call from the kernel).
     */
    pid = P2_Spawn("P3_Startup", P3_Startup, NULL,  4 * USLOSS_MIN_STACK, 3);
    pid = P2_Wait(&status);


    /*
     * Kill the device drivers
     */
    P1_Kill(clockPID);
    // ...

    /*
     * Join with the device drivers.
     */
    // ...
    return 0;
}

static int
ClockDriver(void *arg)
{
    P1_Semaphore running = (P1_Semaphore) arg;
    int result;
    int status;
    int rc = 0;

    /*
     * Let the parent know we are running and enable interrupts.
     */
    P1_V(running);
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
    while(1) {
        result = P1_WaitDevice(USLOSS_CLOCK_DEV, 0, &status);
        if (result != 0) {
            rc = 1;
            goto done;
        }
        /*
         * Compute the current time and wake up any processes
         * whose time has come.
         */
    }
done:
    return rc; 
}


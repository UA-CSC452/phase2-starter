
#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <assert.h>
#include <libuser.h>
#include <libdisk.h>

#include "phase2Int.h"

int P2_Startup(void *arg)
{
    int rc, pid;

    P2ClockInit();
    P2DiskInit();

    // install system call handlers

    rc = P2_Spawn("P3_Startup", P3_Startup, NULL, 4*USLOSS_MIN_STACK, 2, &pid);
    assert(rc == P1_SUCCESS);
    
    // wait for P3_Startup to terminate

    P2DiskShutdown();
    P2ClockShutdown();

    return 0;
}


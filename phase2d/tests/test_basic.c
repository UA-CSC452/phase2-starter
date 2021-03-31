/*
 * Tests lock and condition variable system calls
 */

#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libuser.h>

#include "tester.h"
#include "phase2Int.h"

static int lock;
static int cond;
static int passed = FALSE;
static int unblocked = FALSE;
static int counter = 0;

static int Proc(void *arg){
    int i = (int) arg;
    int rc;

    char name[P1_MAXNAME];
    USLOSS_Console("Proc%d: Testing lock %d and cond %d get-name.\n", i, lock, cond);
    rc = Sys_LockName(lock, name, sizeof(name));
    TEST(rc, P1_SUCCESS);
    TEST(strcmp(name, "lock0"), 0);
    rc = Sys_CondName(cond, name, sizeof(name));
    TEST(rc, P1_SUCCESS);
    TEST(strcmp(name, "cond0"), 0);

    rc = Sys_LockAcquire(lock);
    TEST(rc, P1_SUCCESS);

    if(i != 4){
        USLOSS_Console("Proc%d: Calling CondWait\n", i);
        while(unblocked == FALSE) {
            rc = Sys_CondWait(cond);
            TEST(rc, P1_SUCCESS);
        }
    } else {
        USLOSS_Console("Proc%d: Unblocking the blocked processes\n", i);
        unblocked = TRUE; 
        for(int j = 0; j < 4; j++){
            rc = Sys_CondSignal(cond);
            TEST(rc, P1_SUCCESS);
        }
    }
    counter++;
    USLOSS_Console("Proc%d: Work done. Quitting.\n", i);
    rc = Sys_LockRelease(lock);
    TEST(rc, P1_SUCCESS);
    return 12;
    }


static int
Blocks(void *arg)
{   
    int rc;
    int pid; 
    int status;

    USLOSS_Console("Parent: Lock is %d, Cond is %d\n", lock, cond);

    USLOSS_Console("Parent: Running.\n");
    USLOSS_Console("Parent: Spawning 4 processes Proc1, Proc2, Proc3, and Proc4 with priority 2\n");
    for (int i = 1; i < 5; i++) {
        rc = Sys_Spawn(MakeName("Proc", i), Proc, (void *) i, USLOSS_MIN_STACK, 2, &pid);
        TEST(rc, P1_SUCCESS);
    }
    
    for(int i = 1; i < 5; i++){
        rc = Sys_Wait(&pid, &status);
        TEST(rc, P1_SUCCESS); 
        TEST(status, 12);
    }
    TEST(counter, 4);
    USLOSS_Console("Parent: Freeing the lock and cond var.\n");
    rc = Sys_LockFree(lock);
    TEST(rc, P1_SUCCESS);
    rc = Sys_CondFree(cond);
    TEST(rc, P1_SUCCESS);
    USLOSS_Console("Parent: Quitting.\n");
    return 0;
}

int 
P3_Startup(void *arg)
{
    int rc;
    int pid;
    int status;
    
    USLOSS_Console("P3_Startup: Creating lock and condition variable\n");
    rc = Sys_LockCreate("lock0", &lock);
    TEST(rc, P1_SUCCESS);
    rc = Sys_CondCreate("cond0", lock, &cond);
    TEST(rc, P1_SUCCESS);
    USLOSS_Console("Lock %d and Cond %d created\n", lock, cond);

    USLOSS_Console("P3_Startup: Spawning Parent process with priority 3\n");
    rc = Sys_Spawn("Blocks", Blocks, NULL, USLOSS_MIN_STACK, 3, &pid);
    TEST(rc, P1_SUCCESS);    
    
    rc = Sys_Wait(&pid, &status);
    TEST(rc, P1_SUCCESS);
    passed = TRUE;
    return 0;
}


void test_setup(int argc, char **argv) {}

void test_cleanup(int argc, char **argv) {
     if (passed){
	   PASSED_MSG();
     }
}

void finish(int argc, char **argv) {}

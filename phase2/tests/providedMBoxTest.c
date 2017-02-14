#include <assert.h>
#include <usloss.h>
#include <stdlib.h>
#include <stdarg.h>
#include <phase1.h>
#include <stdio.h>
#include "libuser.h"


static int mbox;

int Recvr(void *arg) {
    int i = 0;
    for (i = 0; i < 10; i++) {
        int buf;
        int size = sizeof(int);
        int result = Sys_MboxReceive(mbox, &buf, &size);
        assert(result == 0);
        assert(size == sizeof(int));
        assert((buf >= 0 && buf < 10));
    }
    return 0;
}

int Sender(void *arg) {
    int i;
    for(i = 0; i < 10; i++) {
    	USLOSS_Console("Sending: %d\n", i);
        int size = sizeof(int);
        int result = Sys_MboxSend(mbox, &i, &size);
        assert(result == 0);
    }
    return 0;
}

int
P3_Startup(void *arg)
{ 
    int pid;
    int result;
    int i;
    char name[30];
    int status;

    result = Sys_MboxCreate(100, 4, &mbox);
    assert(mbox != -1);
    assert(result == 0);
    for (i = 0; i < 3; i++) {
        snprintf(name, sizeof(name), "Receiver %d", i);
        result = Sys_Spawn(name, Recvr, NULL, USLOSS_MIN_STACK, 4, &pid);
        assert(pid >= 0);
        assert(result == 0);
    }
    for (i = 0; i < 3; i++) {
        snprintf(name, sizeof(name), "Sender %d", i);
        result = Sys_Spawn(name, Sender, NULL, USLOSS_MIN_STACK, 4, &pid);
        assert(pid >= 0);
        assert(result == 0);
    }
    for (i = 0; i < 6; i++) {
        result = Sys_Wait(&pid, &status);
        assert(result == 0);
    }
    return 0;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}

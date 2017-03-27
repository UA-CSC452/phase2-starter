/*
 *
 *  Created on: Mar 8, 2015
 *      Author: jeremy
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include <libuser.h>
#include <libdisk.h>

#define MSG "This is a test."

int P3_Startup(void *arg) {
    char buffer[USLOSS_DISK_SECTOR_SIZE];
    strncpy(buffer, MSG, sizeof(buffer));
    int status;
    USLOSS_Console("Write to the disk.\n");
    int result = Sys_DiskWrite(buffer, 0, 0, 1, 0, &status);
    USLOSS_Console("Verify that the disk write was successful.\n");
    assert(result == 0);
    assert(status == 0);
    USLOSS_Console("Wrote \"%s\".\n", buffer);
    bzero(buffer, sizeof(buffer));
    status = -12;
    USLOSS_Console("Read from the disk.\n");
    result = Sys_DiskRead(buffer, 0, 0, 1, 0, &status);
    USLOSS_Console("Verify that the disk read was successful.\n");
    assert(result == 0);
    assert(status == 0);
    assert(!strcmp(MSG, buffer));
    USLOSS_Console("Read \"%s\".\n", buffer);
    USLOSS_Console("You passed all the tests! Treat yourself to a cookie!\n");
    return 7;
}

void test_setup(int argc, char **argv) {
    int rc;

    rc = Disk_Create(NULL, 0, 10);
    assert(rc == 0);
}

void test_cleanup(int argc, char **argv) {
    unlink("./disk0");
}

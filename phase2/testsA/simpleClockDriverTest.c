/*
 * simpleClockDriverTest.c
 *
 *  Created on: Mar 9, 2015
 *      Author: tishihara
 */

#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include <libuser.h>

int P3_Startup(void *arg) {
	int rc;

	USLOSS_Console("Testing one call to Sys_Sleep\n");
	USLOSS_Console("Sleeping for 3 seconds...\n");
	int initTime;
	Sys_GetTimeOfDay(&initTime);
	rc = Sys_Sleep(3);
	assert(rc == 0);
	int finalTime;
	Sys_GetTimeOfDay(&finalTime);
	USLOSS_Console("Total time: %f seconds\n", (finalTime-initTime)/1000000.0);
	USLOSS_Console("Woke up!\n");
	USLOSS_Console("You passed the test! Treat yourself to a cookie!\n");
	return 7;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}

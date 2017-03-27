/**
   This tests the terminal write to unit 0.
 **/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include <libuser.h>

#define MSG "This is a test.\n"

int P3_Startup(void *arg) {
  int n;
  char buffer[P2_MAX_LINE];
  int rc;

  USLOSS_Console("P3_Startup:\n");
  USLOSS_Console("Calling Sys_TermWrite on unit 0\n");

  bzero(buffer, sizeof(buffer));
  rc = Sys_TermWrite(MSG, strlen(MSG), 0, &n);
  assert(rc == 0);
  assert(n == strlen(MSG));
  FILE *f = fopen("term0.out", "r");
  n = fread(buffer, 1, sizeof(buffer), f);
  fclose(f);
  assert(n == strlen(MSG));
  assert(!strcmp(buffer, MSG));
  USLOSS_Console("Test passed.\n");
  return 0;
}

void test_setup(int argc, char **argv) {
}

void test_cleanup(int argc, char **argv) {

}
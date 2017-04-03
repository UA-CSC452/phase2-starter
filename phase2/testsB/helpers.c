
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <usloss.h>
#include <string.h>
#include "helpers.h"

void
DeleteTerm(int term, char *suffix)
{
    char path[30];
    snprintf(path, sizeof(path), "term%d.%s", term, suffix);
    unlink(path);
}

void 
DeleteTerms(char *suffix)
{
    for (int i = 0; i < USLOSS_TERM_UNITS; i++) {
        DeleteTerm(i, suffix);
    }
}

void
DeleteAllTerms(void)
{
    DeleteTerms("in");
    DeleteTerms("out");
}

FILE *
OpenTermFile(int term, char *suffix, char *mode) 
{
    char path[30];
    FILE *f;

    snprintf(path, sizeof(path), "term%d.%s", term, suffix);
    f = fopen(path, mode);
    return f;
}

void
CreateTermInput(int term, char *input) 
{
    FILE *f;
    
    f = OpenTermFile(term, "in", "w");
    fprintf(f, "%s", input);
    fclose(f);
}

void
CreateTermInputFromArray(int term, int count, char *inputs[]) 
{
    FILE *f;
    
    f = OpenTermFile(term, "in", "w");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%s", inputs[i]);
    }
    fclose(f);
}

void
CheckTermOutput(int term, char *expected) 
{
    int size = strlen(expected) + 1;
    char *buffer = malloc(size);
    FILE *f;
    int n;

    f = OpenTermFile(term, "out", "r");
    bzero(buffer, size);
    n = fread(buffer, 1, strlen(expected), f);
    assert(n == strlen(expected));
    assert(!strcmp(expected, buffer));
    free(buffer);
    fclose(f);
}

void
DeleteDisk(int disk) 
{
    char path[30];
    snprintf(path, sizeof(path), "disk%d", disk);
    unlink(path);
}

void
DeleteAllDisks(void)
{
    for (int i = 0; i < USLOSS_DISK_UNITS; i++) {
        DeleteDisk(i);
    }
}



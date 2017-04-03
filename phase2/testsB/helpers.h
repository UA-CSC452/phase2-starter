#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <stdio.h>

extern void DeleteTerm(int term, char *suffix);
extern void DeleteTerms(char *suffix);
extern void DeleteAllTerms(void);
extern FILE *OpenTermFile(int term, char *suffix, char *mode);
extern void CreateTermInput(int term, char *input);
extern void CreateTermInputFromArray(int term, int count, char *inputs[]);
extern void CheckTermOutput(int term, char *expected);
extern void DeleteDisk(int disk);
extern void DeleteAllDisks(void);

#endif
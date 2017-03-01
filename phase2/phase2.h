/*
 * These are the definitions for phase0 of the project (the kernel).
 * DO NOT MODIFY THIS FILE.
 *
 * Version: 2.0
 */

#ifndef _PHASE2_H
#define _PHASE2_H

/*
 * Maximum line length
 */

#define P2_MAX_LINE	72

#define P2_MAX_MBOX 	200

/* 
 * Function prototypes for this phase.
 */

#ifndef CHECKRETURN
#define CHECKRETURN __attribute__((warn_unused_result))
#endif

extern  int	    P2_Sleep(int seconds) CHECKRETURN;

extern	int	    P2_TermRead(int unit, int size, char *buffer) CHECKRETURN;
extern  int	    P2_TermWrite(int unit, int size, char *text) CHECKRETURN;

extern  int     P2_DiskRead(int unit, int track, int first, int sectors, void *buffer) CHECKRETURN;
extern  int	    P2_DiskWrite(int unit, int track, int first, int sectors, void *buffer) CHECKRETURN;
extern  int 	P2_DiskSize(int unit, int *sector, int *track, int *disk) CHECKRETURN;

extern  int     P2_Spawn(char *name, int (*func)(void *arg), void *arg, int stackSize, 
                         int priority) CHECKRETURN;
extern  int     P2_Wait(int *status) CHECKRETURN;
extern  void    P2_Terminate(int status);

extern	int 	P3_Startup(void *) CHECKRETURN;

extern	int     P2_MboxCreate(int slots, int size) CHECKRETURN;
extern	int	    P2_MboxRelease(int mbox) CHECKRETURN;
extern	int	    P2_MboxSend(int mbox, void *msg, int *size) CHECKRETURN;
extern	int	    P2_MboxCondSend(int mbox, void *msg, int *size) CHECKRETURN;
extern  int	    P2_MboxReceive(int mbox, void *msg, int *size) CHECKRETURN;
extern  int     P2_MboxCondReceive(int mbox, void *msg, int *size) CHECKRETURN;

#endif /* _PHASE2_H */

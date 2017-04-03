/*
 * Stress test. 
 *      - term 0 copied to term 3 by several processes
 *      - term 0 copied to 1st half of disk 0
 *      - term 1 copied to itself
 *      - disk 0 copied to itself and to disk 1
 *      - disk 1 copied to term 2
 *      - disk 1 is read and written with binary data
 *      - term 3 is copied to term 0 by several processes
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
#include <stdio.h>
#include "helpers.h"

// Here are some things you can change.

#define NUM_TRACKS 20 // must be a multiple of 2

#define NUM_TERM_COPIERS 3 // # children copying from term 0 to term 3

// You probably shouldn't change any of these macros.

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define INVALID_MBOX -1

#define INPUT_SIZE (USLOSS_DISK_SECTOR_SIZE * USLOSS_DISK_TRACK_SIZE * (NUM_TRACKS/2))

int termLines[USLOSS_TERM_UNITS];

static int verbosity = 0;

#define Debug(format, args...) \
    if (verbosity >= 2) { \
        int pid; \
        Sys_GetPID(&pid); \
        USLOSS_Console("[%s:%d] (%d) " format, __FUNCTION__, __LINE__, pid, ##args); \
    }

#define Info(format, args...) \
    if (verbosity >= 1) { \
        int pid; \
        Sys_GetPID(&pid); \
        USLOSS_Console("[%s:%d] (%d) " format, __FUNCTION__, __LINE__, pid, ##args); \
    }

static int cleanup = 1; // cleanup all test-related files

int diskReady[USLOSS_DISK_UNITS];  // semaphores

static void
SendMsg(int mbox, int size, void *msg) {
    int rc;
    int sent;

    sent = size;
    rc = Sys_MboxSend(mbox, msg, &sent);
    assert(rc == 0);
    assert(size == sent);
    Debug("sent %d bytes to mbox %d\n", sent, mbox);
}

static void
ReceiveMsg(int mbox, int *size, void *msg) {
    int rc;
    int received;

    received = *size;
    rc = Sys_MboxReceive(mbox, msg, &received);
    assert(rc == 0);
    assert(received <= *size);
    *size = received;
    Debug("received %d bytes from mbox %d\n", received, mbox);
}


/*
 * CopyTerm0ToMailboxes
 *
 * Reads lines from term 0 and sends them to the specified mailboxes. The mailboxes
 * must be terminated by -1. Sends an empty message after the last line.
 */
static int
CopyTerm0ToMailboxes(void *arg) {
    int     *mboxes = (int *) arg;
    char    buffer[P2_MAX_LINE+1]; // +1 for null character
    int     bytesRead;
    int     rc;

    Info("CopyTerm0ToMailboxes starting.\n");

    for (int lines = 0; lines < termLines[0]; lines++) {
        // Read a line from term 0.
        bzero(buffer, sizeof(buffer));
        rc = Sys_TermRead(buffer, sizeof(buffer), 0, &bytesRead);
        assert(rc == 0);
        assert(bytesRead <= sizeof(buffer));

        Debug("read: %s", buffer);

        // Send it to the mailboxes.

        for (int i = 0; mboxes[i] != -1; i++) {
            SendMsg(mboxes[i], bytesRead, buffer);                
        }
    }

    // Send empty message for EOF.
    for (int i = 0; mboxes[i] != -1; i++) {
        SendMsg(mboxes[i], 0, NULL);                
    }
    Info("CopyTerm0ToMailboxes done.\n");
    return 15;
}


/*
 * WriteLinesToTerm3
 *
 * Writes lines from the mailbox to term 3.
 */

static int
WriteLinesToTerm3(void *arg) {
    int     mbox = (int) arg;
    char    line[P2_MAX_LINE+1]; // +1 for null character
    int     size;
    int     rc;

    Info("WriteLinesToTerm3 starting.\n");
    while (1) {
        bzero(line, sizeof(line));
        size = sizeof(line);
        ReceiveMsg(mbox, &size, line); 

        if (size == 0) {
            // EOF, let the others know. This is a bit of a hack -- assumes the mailbox
            // is large enough so that this doesn't block. 
            for (int i = 0; i < NUM_TERM_COPIERS - 1; i++) {
                SendMsg(mbox, 0, NULL);
            }
            break;
        }

        // Write it to term 3

        int bytesWritten = size;
        rc = Sys_TermWrite(line, size, 3, &bytesWritten);
        assert(rc == 0);
        assert(bytesWritten == size);

        Debug("wrote %d bytes to term 3\n", bytesWritten);

    }
    Info("WriteLinesToTerm3 done.\n");
    return 17;
}

#define MAX_DISK_IO (USLOSS_DISK_SECTOR_SIZE * USLOSS_DISK_TRACK_SIZE * 4)

/*
 * WriteLinesToDisk
 *
 * Writes lines from the mailbox to disk 0.
 */


static int
WriteLinesToDisk(void *arg) {
    int     mbox = (int) arg;
    char    line[P2_MAX_LINE+1]; // +1 for null character
    char    *buffer = NULL;
    int     count = 0; // # bytes written
    int     amount = 0; // # bytes to write in the next disk I/O
    int     track = 0;
    int     first = 0;
    int     finished = 0;
    int     rc;

    Info("WriteLinesToDisk starting.\n");
    buffer = (char *) malloc(MAX_DISK_IO);
    while (!finished) {
        bzero(buffer, MAX_DISK_IO);
        count = 0;

        // Each time we increase the size by one sector. 
        amount += USLOSS_DISK_SECTOR_SIZE;
        amount = MIN(amount, MAX_DISK_IO);

        // Fill the buffer
        while ((!finished) && (count < amount)) {
            bzero(line, sizeof(line));
            int bytesReceived = sizeof(line);
            rc = Sys_MboxReceive(mbox, line, &bytesReceived);
            assert(rc == 0);
            assert(bytesReceived <= P2_MAX_LINE);
            Debug("bytesReceived %d\n", bytesReceived);
            if (bytesReceived > 0) {
                int toCopy = MIN(bytesReceived, amount - count);
                memcpy(buffer + count, line, toCopy);
                count += toCopy;
            } else {
                finished = 1;
            }
            if (finished || (count == amount)) {
                // Write the buffer to disk.
                int sectors = (amount + USLOSS_DISK_SECTOR_SIZE - 1) / USLOSS_DISK_SECTOR_SIZE;
                int status;
                Debug("DiskWrite track %d first %d sectors %d\n", track, first, sectors);
                rc = Sys_DiskWrite((void *) buffer, track, first, sectors, 0, &status);
                assert(rc == 0);
                // Compute the start of the next I/O
                first += sectors;
                track += (first / USLOSS_DISK_TRACK_SIZE);
                first = first % USLOSS_DISK_TRACK_SIZE;
                Debug("DiskWrite new track %d first %d sectors %d\n", track, first, sectors);

            }
        }
    }
    // Let waiting processes know the disk is ready.

    for (int i = 0; i < 2; i++) {
        rc = Sys_SemV(diskReady[0]);
        assert(rc == 0);
    }

    Info("WriteLinesToDisk done.\n");
    return 11;
}

/*
 * LoopBackTerm
 *
 * Copies lines from the specified terminal to itself.
 */
static int
LoopBackTerm(void *arg) {
    int     term = (int) arg;
    char    line[P2_MAX_LINE * 2]; // shouldn't hurt
    int     bytesRead;
    int     bytesWritten;
    int     rc;

    Info("LoopBackTerm starting.\n");
    for (int lines = 0; lines < termLines[term]; lines++) {
        // Read a line from the terminal.
        bzero(line, sizeof(line));
        rc = Sys_TermRead(line, sizeof(line), term, &bytesRead);
        assert(rc == 0);
        assert(bytesRead <= P2_MAX_LINE);

        Debug("read from %d: %s", term, line);

        // Write it back to the terminal.

        bytesWritten = bytesRead;
        rc = Sys_TermWrite(line, bytesRead, term, &bytesWritten);
        assert(rc == 0);
        assert(bytesRead == bytesWritten);

        Debug("wrote %d bytes to term %d\n", bytesWritten, term);
    }
    Info("LoopBackTerm done.\n");
    return 14;
}

/*
 * CopyTerm3ToTerm0
 *
 * Reads lines from term 3 and writes them to term 0.
 */
static int
CopyTerm3ToTerm0(void *arg) {
    int     numLines = (int) arg;
    char    line[P2_MAX_LINE+1]; // +1 for null character
    int     bytesRead;
    int     bytesWritten;
    int     rc;

    Info("CopyTerm3ToTerm0 starting.\n");
    for (int lines = 0; lines < numLines; lines++) {

        // Read a line from term 3.
        bzero(line, sizeof(line));
        rc = Sys_TermRead(line, sizeof(line), 3, &bytesRead);
        assert(rc == 0);
        assert(bytesRead <= sizeof(line));

        Debug("read %d bytes\n", bytesRead);

        // Write it to term 0.
        bytesWritten = bytesRead;
        rc = Sys_TermWrite(line, bytesRead, 0, &bytesWritten);
        assert(rc == 0);
        assert(bytesRead == bytesWritten);

        Debug("wrote %d bytes\n", bytesWritten);
    }
    Info("CopyTerm3ToTerm0 done.\n");
    return 10;
}

/*
 * CopyDisk0
 *
 * Copies the 1st half of disk0 to the 2nd half or to disk 1.
 */
static int
CopyDisk0(void *arg) {
    int     disk = (int) arg;
    char    *sector;
    int     rc;
    int     sectorSize;
    int     trackSize;
    int     tracks;
    int     numSectors;
    int     first;
    int     track;
    int     offset; 

    Info("CopyDisk0 starting.\n");
    // Wait for the disk to be ready.

    rc = Sys_SemP(diskReady[0]);
    assert(rc == 0);

    Debug("CopyDisk0 %d running\n", disk);
    rc = Sys_DiskSize(0, &sectorSize, &trackSize, &tracks); 
    assert(rc == 0);
    assert((tracks & 1) == 0); // must be an even # of tracks
    // Copies to the upper half of disk 0 and lower half of disk 1.
    if (disk == 0) {
        offset = tracks/2;
    } else {
        offset = 0;
    }

    sector = malloc(sectorSize);
    numSectors = trackSize * (tracks/2); // copy half of the disk.

    track = 0;
    first = 0;
    for (int i = 0; i < numSectors; i++) {
        int status;

        // Read the next sector of disk 0

        Debug("reading sector %d\n", first);
        bzero(sector, sectorSize);
        rc = Sys_DiskRead(sector, track, first, 1, 0, &status);
        assert(rc == 0);

        // Write it to the destination.

        rc = Sys_DiskWrite(sector, track + offset, first, 1, disk, &status);
        assert(rc == 0);

        Debug("wrote to %d:%d:%d\n", disk, track+offset, first);

        // Update the first sector

        first++;
        if (first == trackSize) {
            track++;
            first = 0;
        }
    }
    if (disk == 1) {
        Debug("Disk 1 is ready.\n");
        rc = Sys_SemV(diskReady[disk]);
        assert(rc == 0);
    }

    Info("CopyDisk0 done.\n");
    return 16;
}

/*
 * CopyDisk1
 *
 * Copies the 1st half of disk1 to the terminal.
 */
static int
CopyDisk1(void *arg) {
    int     term = (int) arg;
    char    *sector;
    int     rc;
    int     sectorSize;
    int     trackSize;
    int     tracks;
    int     numSectors;
    int     first;
    int     track;
    int     status;

    Info("CopyDisk1 starting.\n");

    // Wait for the disk to be ready.

    rc = Sys_SemP(diskReady[1]);
    assert(rc == 0);

    Debug("CopyDisk1 running\n");
    rc = Sys_DiskSize(0, &sectorSize, &trackSize, &tracks); 
    assert(rc == 0);
    assert((tracks & 1) == 0); // must be an even # of tracks
    sector = malloc(sectorSize);
    numSectors = trackSize * (tracks/2); // copy half of the disk.

    track = 0;
    first = 0;
    for (int i = 0; i < numSectors; i++) {
        int status;
        char *ptr;
        int bytesWritten;

        // Read the next sector of disk 1

        Debug("reading sector %d\n", first);
        bzero(sector, sectorSize);
        rc = Sys_DiskRead(sector, track, first, 1, 1, &status);
        assert(rc == 0);

        // Write each line to the terminal.

        ptr = sector;
        while (ptr < sector + sectorSize) {
            char *newline;
            int size;

            Debug("writing to term %d\n", term);
            newline = strchr(ptr, '\n');
            assert(newline != NULL);
            size = newline - ptr + 1;
            rc = Sys_TermWrite(ptr, size, term, &bytesWritten);
            assert(rc == 0);
            assert(size == bytesWritten);
            ptr += bytesWritten;
            Debug("wrote %d bytes to term %d\n", bytesWritten, term);
        }
        // Update the first sector

        first++;
        if (first == trackSize) {
            track++;
            first = 0;
        }
    }
    // Check that we can read and write binary data.

    Debug("reading and writing binary data\n");
    memset(sector, 42, sectorSize);
    rc = Sys_DiskWrite(sector, tracks/2, 0, 1, 1, &status);
    assert(rc == 0);
    char *sector2 = malloc(sectorSize);
    bzero(sector2, sectorSize);
    rc = Sys_DiskRead(sector2, tracks/2, 0, 1, 1, &status);
    assert(rc == 0);
    assert(!memcmp(sector, sector2, sectorSize));
    Info("CopyDisk1 done.\n");
    return 13;
}

int statuses[P1_MAXPROC]; // expected exit status for each child.

#define STACKSIZE (USLOSS_MIN_STACK + 100 * 1024)

int 
P3_Startup(void *arg) {
    int     rc;
    char    buffer[30];
    int     pid;
    int     result = 0;
    int     children = 0; // # of children
    int     mboxes[3];


    Info("P3_Startup starting.\n");
    for (int i = 0; i < USLOSS_DISK_UNITS; i++) {
        snprintf(buffer, sizeof(buffer), "diskReady %d", i);
        rc = Sys_SemCreate(buffer, 0, &diskReady[i]);
        assert(rc == 0);
    }

    // Create mailboxes to which term0 is sent.

    for (int i = 0; i < 3; i++) {
        rc = Sys_MboxCreate(10, P2_MAX_LINE, &mboxes[i]);
        assert(rc == 0);
    }
    mboxes[2] = -1; // terminate list of mailboxes

    // Create child to copy from term 0 to the mailboxes.
    rc = Sys_Spawn("CopyTerm0ToMailboxes", CopyTerm0ToMailboxes, (void *) mboxes, STACKSIZE, 3, &pid);
    assert(rc == 0);
    assert(pid >= 0);
    statuses[pid] = 15;
    children++;

    // Create children to copy from mailbox to term 3.
    for (int i = 0; i < NUM_TERM_COPIERS; i++) {
        snprintf(buffer, sizeof(buffer), "WriteLinesToTerm3 %d", i);
        rc = Sys_Spawn(buffer, WriteLinesToTerm3, (void *) mboxes[0], STACKSIZE, 3, &pid);
        assert(rc == 0);
        assert(pid >= 0);
        statuses[pid] = 17;
        children++;
    }

    // Create child to copy from mailbox to disk 0.
    rc = Sys_Spawn("WriteLinesToDisk", WriteLinesToDisk, (void *) mboxes[1], STACKSIZE, 3, &pid);
    assert(rc == 0);
    assert(pid >= 0);
    statuses[pid] = 11;
    children++;

    // Loopback term 1.
    rc = Sys_Spawn("LoopBackTerm", LoopBackTerm, (void *) 1, STACKSIZE, 3, &pid);
    assert(rc == 0);
    assert(pid >= 0);
    statuses[pid] = 14;
    children++;

    int numLines = termLines[3] / NUM_TERM_COPIERS;
    for (int i = 0; i < NUM_TERM_COPIERS; i++) {
        if (i == NUM_TERM_COPIERS - 1) {
            numLines = termLines[3] - ((NUM_TERM_COPIERS - 1) * numLines);
        }
        snprintf(buffer, sizeof(buffer), "CopyTerm3ToTerm0 %d", i);
        rc = Sys_Spawn(buffer, CopyTerm3ToTerm0, (void *) numLines, STACKSIZE, 3, &pid);
        assert(rc == 0);
        assert(pid >= 0);
        statuses[pid] = 10;
        children++;
    }

    // Copy disk 0 to itself and to disk 1.

    for (int i = 0; i < 2; i++) {
        snprintf(buffer, sizeof(buffer), "CopyDisk0 %d", i);
        rc = Sys_Spawn(buffer, CopyDisk0, (void *) i, STACKSIZE, 3, &pid);
        assert(rc == 0);
        assert(pid >= 0);
        statuses[pid] = 16;
        children++;
    }

    // Copy disk 1 to terminal 2.
    rc = Sys_Spawn("CopyDisk1", CopyDisk1, (void *) 2, STACKSIZE, 3, &pid);
    assert(rc == 0);
    assert(pid >= 0);
    statuses[pid] = 13;
    children++;


    // Wait for all the children to quit.

    for (int i = 0; i < children; i++) {
        int status;
        rc = Sys_Wait(&pid, &status);
        assert(rc == 0);
        assert(status == statuses[pid]);
    }

    // term3.out should be the same as term0.in after sorting
    rc = system("sort -n term3.out > term3.sorted");
    assert(rc == 0);
    rc = system("diff term0.in term3.sorted");
    if (rc != 0) {
        USLOSS_Console("term3.out incorrect.\n");
        result = 1;
        goto done;
    }

    // term0.out should equal term3.in
    rc = system("diff term0.out term3.in");
    if (rc != 0) {
        USLOSS_Console("term0.out incorrect.\n");
        result = 1;
        goto done;
    }


    // term1.out should equal term1.in
    rc = system("diff term1.in term1.out");
    if (rc != 0) {
        USLOSS_Console("term1.out incorrect.\n");
        result = 1;
        goto done;
    }

    // term2.out should equal term1.in
    rc = system("diff term1.in term2.out");
    if (rc != 0) {
        USLOSS_Console("term2.out incorrect.\n");
        result = 1;
        goto done;
    }


    // disk 0 should be term0.in repeated

    rc = system("cat term0.in term0.in > disk0.correct");
    assert(rc == 0);
    rc = system("diff disk0 disk0.correct");
    if (rc != 0) {
        USLOSS_Console("disk0 incorrect.\n");
        result = 1;
        goto done;
    }
    // disk 1 should start with term0.in
    char command[100];
    snprintf(command, sizeof(command), "head -c %d disk1 > disk1.head", INPUT_SIZE);
    rc = system(command);
    assert(rc == 0);
    rc = system("diff term0.in disk1.head");
    if (rc != 0) {
        USLOSS_Console("disk1 incorrect.\n");
        result = 1;
        goto done;
    }

done:
    if (result) {
        Info("Tests failed.\n");
    } else {
        Info("Tests passed.\n");   
    }
    Info("P3_Startup done.\n");
    return result;
}

static void
Usage(char *cmd)
{
    fprintf(stderr, "Usage: %s [-vCh]\n", cmd);
    fprintf(stderr, "\t-v\t\tIncrease verbosity.\n");
    fprintf(stderr, "\t-C\t\tDo not clean up files (useful for debugging).\n");
    fprintf(stderr, "\t-h\t\tPrint this message.\n");
}

void 
test_setup(int argc, char **argv) {
    int rc;
    int opt;

    while ((opt = getopt(argc, argv, "vCh")) != -1) {
        switch(opt) {
            case 'v':
                verbosity++;
                break;
            case 'C':
                cleanup = 0;
                break;
            case 'h':
                Usage(argv[0]);
                exit(0);
                break;
            case '?':
            default:
                Usage(argv[0]);
                exit(1);
        }
  }
    if (optind < argc) {
        Usage(argv[0]);
        exit(1);
    }

    DeleteAllDisks();
    for (int i = 0; i < USLOSS_DISK_UNITS; i++) {
        rc = Disk_Create(NULL, i, NUM_TRACKS);
        assert(rc == 0);
    }

    // Create term0.in equal to half the disk size (10 tracks). 

    int written = 0;
    int i = 0;
    FILE *f = OpenTermFile(0, "in", "w");
    assert(f != NULL);

    while (written < INPUT_SIZE) {
        char buffer[30];
        int toWrite;
        int n;

        bzero(buffer, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%04d Bear Down!\n", i);
        toWrite = strlen(buffer);
        toWrite = MIN(toWrite, INPUT_SIZE - written);
        // Make sure line is newline-terminated (happens if line is not a power of 2). 
        buffer[toWrite - 1] = '\n';
        n = fwrite(buffer, 1, toWrite, f);
        assert(n == toWrite);
        written += n;
        i++;
    }
    fclose(f);

    bzero(termLines, sizeof(termLines));
    // Record the # of the last line we read so that we can find the terminal input end
    termLines[0] = i;

    // Copy term0.in to term1.in and term3.in
    rc = system("cp term0.in term1.in");
    assert(rc == 0);
    termLines[1] = termLines[0];
    rc = system("cp term0.in term3.in");
    assert(rc == 0);
    termLines[3] = termLines[0];

}

void 
test_cleanup(int argc, char **argv) {
    if (cleanup) {
        system("rm -f disk[0-1] disk[0-1].*");
        system("rm -f term[0-4].*");
    }
}

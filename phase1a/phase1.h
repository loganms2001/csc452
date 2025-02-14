// These are the definitions for phase1 of the project (the kernel).

#ifndef _PHASE1_H
#define _PHASE1_H

#include <usloss.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
// #include <stdint.h>
#include <string.h>

#define MAXPROC 50
#define MAXNAME 50
#define MAXSYSCALLS 50


// These functions must be provided by Phase 1.

// Big deals
extern void phase1_init(void);
extern int  spork(char *name, int (*)(void *), void *arg, int stacksize, int priority);
extern int  join(int *status);

// Modified in 1b
extern void quit_phase_1a(int status, int switchToPid) __attribute__((__noreturn__));
extern void quit         (int status)                  __attribute__((__noreturn__));
// extern void zap(int pid);
// extern void blockMe();
// extern int unblockProc(int pid);

// Debug functions
extern int  getpid(void);
extern void dumpProcesses(void);

// Temp functions
void TEMP_switchTo(int pid);


/*
* These functions are called *BY* Phase 1 code, and are implemented in
* Phase 5.  If we are testing code before Phase 5 is written, then the
* testcase must provide a NOP implementation of each.
*/
extern USLOSS_PTE *phase5_mmu_pageTable_alloc(int pid);
extern void        phase5_mmu_pageTable_free (int pid, USLOSS_PTE*);


/* these functions are also called by the phase 1 code, from inside
* init_main().  They are called first; after they return, init()
* enters an infinite loop, just join()ing with children forever.
*
* In early phases, these are provided (as NOPs) by the testcase.
*/
extern void phase2_start_service_processes(void);
extern void phase3_start_service_processes(void);
extern void phase4_start_service_processes(void);
extern void phase5_start_service_processes(void);


/* this function is called by the init process, after the service
* processes are running, to start whatever processes the testcase
* wants to run.  This may call spork() many times, and
* block as long as you want.  When it returns, Halt() will be
* called by the Phase 1 code (nonzero means error).
*/
extern int testcase_main(void);


///////////////////////////////////////////////////////////////////////////////////////


// Chow and Logan definitions

// Process statuses
#define BLOCKED      -2
#define NOT_READY     -1
#define READY        0
#define RUNNING      1
#define TERMINATED   2
#define AVAILABLE    3

// other defs
#define PID_UNUSED  -1

// Process structure
typedef struct process {
    int pid;
    char name[MAXNAME];
    int priority;
    int status;
    int exitStatus;
    int stackSize;
    void *stack;
    int (*startFunc)(void *);
    void *arg;
    USLOSS_Context context;
    struct process *parent;
    struct process *firstChild;
    struct process *nextSibling;
    struct process *nextRun;
} Process;

// helper functions
extern int init_entry(void *);
unsigned int set_psr_flag(unsigned int flag, bool on);
extern void func_wrapper(void);
void switch_context(int old_pid, int status, int new_pid);

#endif // _PHASE1_H

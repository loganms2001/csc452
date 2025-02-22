// Chow and Logan definitions

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
// #include <stdint.h>
#include <string.h>
#include <time.h>
#include "phase1.h"

// Process statuses
#define BLOCKED      -1
#define READY        0
#define RUNNING      1
#define TERMINATED   2
#define AVAILABLE    3

// Blocked statuses - just for you, Russ
#define BLOCK_ON_ZAP 1
#define BLOCK_ON_CHILD 2
#define BLOCK_ON_COMMAND 3

// Process structure
typedef struct process {
    int pid;
    char name[MAXNAME];
    int priority;
    int status;
    int blockStatus;
    int exitStatus;
    int stackSize;
    void *stack;
    int (*startFunc)(void *);
    void *arg;
    clock_t startTime;
    int zapList[MAXPROC];
    USLOSS_Context context;
    struct process *parent;
    struct process *firstChild;
    struct process *nextSibling;
    struct process *nextReady;
} Process;

// func decs
void dispatcher(void);
// helper functions
extern int init_entry(void *);
extern void func_wrapper(void);
void switch_context(Process *newProcess);
unsigned int set_psr_flag(unsigned int flag, bool on);
unsigned int disable_interrupts(void);
unsigned int enable_interrupts(void);
void restore_psr_state(unsigned int);
void check_kernel_true(const char *);
bool check_no_children(void);
void enter_kernel_mode(void);
void insert_priority_tail(Process *, Process *);
bool quantum_expired(void);
void blockMe_wrapper(int);

// other defs
#define INIT_PID 1
#define QUANTUM_MS 80
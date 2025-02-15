// Chow and Logan definitions

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
// #include <stdint.h>
#include <string.h>
#include "phase1.h"

// Process statuses
// #define NOT_READY     -1
#define BLOCKED      -1
#define READY        0
#define RUNNING      1
#define TERMINATED   2
#define AVAILABLE    3

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
extern void func_wrapper(void);
void switch_context(int old_pid, int status, int new_pid);
unsigned int set_psr_flag(unsigned int flag, bool on);
unsigned int disable_interrupts(void);
unsigned int enable_interrupts(void);
void restore_psr_state(unsigned int);
void check_kernel_true(const char *);
bool check_no_children(void);
void enter_kernel_mode(void);
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct Process // all subject to change
{
    int pid;
    int (*main_func)(void*); // I think that's how you make a function pointer haha
    struct Process *parent;
    struct Process *first_child;
    struct Process *following_sibling;
};

void phase1_init(void);

int spork(char *name, int (*startFunc)(void*), void *arg, int stackSize, int priority);

int join(int *status);

void quit(int status);

void zap(int pid);

int getpid(void);

void dumpProcesses(void);

void blockMe();

int unblockProc(int pid);
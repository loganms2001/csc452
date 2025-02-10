#include "phase1.h"

Process processTable[MAXPROC];

int main() {
    struct Process a;
    phase1_init();
    puts("compiles and runs");
}

void phase1_init(void) {
	// initialize all slots in process table as -1 (not used)
	for (int i = 0; i < MAXPROC; i++) {
		processTable[i].pid = -1;
	}

	// Initialize init process (PID 1)
	strcpy(processTable[1].name, "init");
	processTable[1].pid = 1;
	processTable[1].priority = 6;
	processTable[1].status = READY;
}

// Creates a new process, which is a child of the currently running process
// - creates an entry in the process table and fills it before calling dispatcher
// - if child is higher priority than parent, child will run before spork() returns
int spork(char *name, int (*startFunc)(void*), void *arg, int stackSize, int priority) {
	// check for invalid args
	if (strlen(name) > MAXNAME || !startFunc || priority > 5 || priority < 1) {
		return -1;
	}
	// check for invalid stack size
	if (stackSize < USLOSS_MIN_STACK) {
		reuturn -2;
	}

	// look for empty slot in process table
	int pid = -1;
	for (int i = 0; i < MAXPROC; i++) {
		if (processTable[i].status == UNUSED) {
			pid = i;
			break;
		}
	}

	if (pid == -1) {
		return -1;
	}	

	// allocate a new slot in the table and use it to store info about
	// the process
	processTable[pid].stack = malloc(stackSize);
	processTable[pid].pid = pid;
	processTable[pid].priority = priority;
	strncpy(processTable[pid].name, name, MAXNAME);
	processTable[pid].status = READY;
	processTable[pid].parent = &processTable[getpid()];  // getpid() returns current (parent) pid
	processTable[pid].first_child = NULL;
	processTable[pid].following_sibling = NULL;

	// call USLOSS_ContextInit() to create a new context for the new process (stores the state)
	USLOSS_ContextInit(&processTable[pid].context, processTable[pid].stack, stackSize, NULL, startFunc);

	return pid;  // return new process pid
}

// Blocks the current process, until one of its children terminates and recieves its exit status.
// - (Phase 1a) In this phase, join() does not block. It returns if no terminated children are found
// 	- also returns if parent has no children 
int join(int *status) {
	// check for invalid status
	if (status == NULL) {
		return -3;
	}

	int parentPID = getpid();
	// use child and previous child to look for terminated children
	struct Process *child = processTable[parentPID].first_child;
	struct Process *previousChild = NULL;

	// look for terminated child
	while (child) {
		if (child->status == TERMINATED) {
			// get the child's exit status
			*status = child->status;

			int childPID = child->pid;

			// remove child
			if (previousChild) {
				previousChild->following_sibling = child->following_sibling;
			} else {
				processTable[parentPID].first_child = child->following_sibling;
			}

			free(child->stack);
			child->stack = NULL;

			// mark as unused in process tabel
			child->pid = -1;
			child->status = UNUSED;
			return childPID;
		}
		previousChild = child;
		child = child->following_sibling;
	}
	
	return -2;  // no children
}

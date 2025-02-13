#include <string.h>
#include "phase1.h"
#include <stdio.h>
#define STACK_SIZE 1000 // Predefined stack size

<<<<<<< Updated upstream
struct Process *processTable[MAXPROC];
Process *currentProcess = NULL;
int availableProcSlots = MAXPROC;
unsigned int PID = 1;

void phase1_init(void) {
	// initialize all processes in process table with pid
	for (int i = 0; i < MAXPROC; i++) {
		processTable[i]->pid = PID_UNUSED;
		processTable[i]->status = AVAILABLE;
	}

	// Initialize init process (PID 1)
	int stackSize = USLOSS_MIN_STACK;
	void *stack = malloc(stackSize);
	USLOSS_PTE *pageTable = phase5_mmu_pageTable_alloc(PID);
	USLOSS_Context *context;
	USLOSS_ContextInit(context, stack, stackSize, pageTable, func_wrapper);

	strcpy(processTable[PID]->name, "init");
	processTable[PID]->pid = PID;
	processTable[PID]->priority = 6; // priority for init
	processTable[PID]->status = READY;
	processTable[PID]->stackSize = stackSize;
	processTable[PID]->stack = stack;
	processTable[PID]->startFunc = init_entry;
	processTable[PID]->arg = NULL;

	processTable[PID]->context = context;

	processTable[PID]->parent = NULL;
	processTable[PID]->firstChild = NULL;
	processTable[PID]->nextSibling = NULL;
	processTable[PID]->nextRun = NULL;

	availableProcSlots--;
=======
char processStacks[MAXPROC][STACK_SIZE];
struct Process processTable[MAXPROC];

// Global variable that holds current pid
int currentPid = 1;

// Function to initialize all process table entries
void initProcessTable(void) {
	for (int i = 0; i < MAXPROC; i++) {
		processTable[i].pid = -1;
        	processTable[i].status = UNUSED;  // Assumes UNUSED is defined as 0.
        	processTable[i].first_child = NULL;
        	processTable[i].following_sibling = NULL;
        	processTable[i].parent = NULL;
        	processTable[i].exit_status = 0;
        	// Initialize name to empty string.
        	processTable[i].name[0] = '\0';
	}
}

int getpid(void) {
	return currentPid;
}

void wrapperFunc() {
        int pid = getpid();
        struct Process *current = &processTable[pid];

        if (current->main_func) {
                int exit_status = current->main_func(current->arg);
                quit(exit_status);
        }

        quit(0);
}
/**
int main() {
	test_init();
	test_spork();
	test_join();
	return 0;
}
*/
void quit(int status) {
	printf("Process %d quitting with status %d\n", getpid(), status);
	processTable[getpid()].status = TERMINATED;
	
	USLOSS_Halt(status);

	while(1);
}

void phase1_init(void) {
	initProcessTable();

	int pid = 1;

	struct Process *initProc = &processTable[pid];
	
	initProc->pid = pid;
	initProc->priority = 6;
	initProc->status = READY;
	strncpy(initProc->name, "init", MAXNAME);

	// Allocate stack space
	initProc->stack = processStacks[pid];

	initProc->main_func = NULL;
	initProc->arg = NULL;	

	USLOSS_ContextInit(&initProc->context, initProc->stack, USLOSS_MIN_STACK, NULL, wrapperFunc);

	// Set current pid to 1
	currentPid = pid;

	// Debugging info
    	USLOSS_Console("init(): Manually initialized PID 1.\n");
>>>>>>> Stashed changes
}

// Creates a new process, which is a child of the currently running process
// - creates an entry in the process table and fills it before calling dispatcher
// - if child is higher priority than parent, child will run before spork() returns
<<<<<<< Updated upstream
int spork(char *name, int (*startFunc)(void *), void *arg, int stackSize, int priority) {
=======
int spork(char *name, int (*startFunc)(void*), void *arg, int stackSize, int priority) {
	int parentPid = getpid();

>>>>>>> Stashed changes
	// check for invalid args
	if (strlen(name) > MAXNAME || !startFunc || priority > 5 || priority < 1) {
		// error message?
		return -1;
	}

	// check to make sure there is room in the process table
	if (availableProcSlots < 1) {
		// error message?
		return -1;
	}

	// check for invalid stack size
	if (stackSize < USLOSS_MIN_STACK) {
		return -2;
	}

	// look for empty slot in process table
<<<<<<< Updated upstream
	while (processTable[PID % MAXPROC]->status != AVAILABLE) {
		PID++;
	}

	// complete process table entry
	void *stack = malloc(stackSize);
	USLOSS_PTE *pageTable = phase5_mmu_pageTable_alloc(PID);
	USLOSS_Context *context;
	USLOSS_ContextInit(context, stack, stackSize, pageTable, func_wrapper);

	strncpy(processTable[PID]->name, name, MAXNAME);
	processTable[PID]->pid = PID;
	processTable[PID]->priority = priority;
	processTable[PID]->status = READY;
	processTable[PID]->stackSize = stackSize;
	processTable[PID]->stack = stack;
	processTable[PID]->startFunc = startFunc;
	processTable[PID]->arg = arg;

	processTable[PID]->context = context;
=======
	int pid = -1;
	for (int i = 2; i < MAXPROC; i++) {
		if (processTable[i].status == UNUSED) {
			pid = i;
			break;
		}
	}

	if (pid == -1) {
		return -1;
	}	
	
	struct Process *newProc = &processTable[pid];

	newProc->pid = pid;
	newProc->priority = priority;
	newProc->status = READY;
	newProc->main_func = startFunc;
	newProc->arg = arg;
	newProc->stack = processStacks[pid];
	strncpy(newProc->name, name, MAXNAME);

	newProc->parent = &processTable[parentPid];
	newProc->first_child = NULL;
	newProc->following_sibling = NULL;

	// call USLOSS_ContextInit() to create a new context for the new process (stores the state)
	// - pass the wrapper function instead of the actual function
	USLOSS_ContextInit(&newProc->context, newProc->stack, stackSize, NULL, wrapperFunc);
>>>>>>> Stashed changes

	processTable[PID]->parent = currentProcess;
	processTable[PID]->firstChild = NULL;
	// nextSibling is defined below
	// nextRun is defined below
	
	availableProcSlots--;
	
	// inserts at head
	Process *temp = currentProcess->firstChild;
	currentProcess->firstChild = processTable[PID];
	processTable[PID]->firstChild = temp;
	
	// inserts at tail
	// processTable[PID]->nextSibling = NULL;
	// Process *child = currentProcess->firstChild;
	// while (child != NULL) {
		// 	child = child->nextSibling;
		// }
		// child->nextSibling = processTable[PID];
		
	Process *cur = NULL;
	Process *next = currentProcess;
	while (next->priority >= priority) {
		cur = next;
		next = next->nextRun;
	}
	if (cur != NULL) {
		cur->nextRun = processTable[PID];
	}
	processTable[PID]->nextRun = next;

	// call dispatcher in 1b

	return PID;  // return new process pid
}

// Blocks the current process, until one of its children terminates and recieves its exit status.
// - (Phase 1a) In this phase, join() does not block. It returns if no terminated children are found
// 	- also returns if parent has no children 
int join(int *status) { // need to audit
	// check for invalid status
	if (status == NULL) {
		return -3;
	}

	int parentPID = getpid();
	// use child and previous child to look for terminated children
	Process *child = processTable[parentPID]->firstChild;
	Process *previousChild = NULL;

	// look for terminated child
	while (child) {
		if (child->status == TERMINATED) {
			// get the child's exit status
			*status = child->exit_status;

			int childPID = child->pid;

			// remove child
			if (previousChild) {
				previousChild->nextSibling = child->nextSibling;
			} else {
				processTable[parentPID]->firstChild = child->nextSibling;
			}
			child->stack = NULL;

			// mark as unused in process table
			child->pid = -1;
			child->status = PID_UNUSED;
			return childPID;
		}
		previousChild = child;
		child = child->nextSibling;
	}
	
	return -2;  // no children
}

<<<<<<< Updated upstream
extern void quit_phase_1a(int status, int switchToPid) {

}

extern int  getpid(void) {

}

extern void dumpProcesses(void) {

}

void TEMP_switchTo(int pid) {

}

////////////////  HELPERS  ////////////////

int init_entry(void *) {
	int new_pid = spork("testcase_main", testcase_main, NULL, USLOSS_MIN_STACK, 3);
	switch_context(currentProcess->pid, READY, new_pid);
	while (true) {
		// join and check for -2
	}
	return 0; // ?
}

// USLOSS_PSR_CURRENT_MODE
// USLOSS_PSR_CURRENT_INT
unsigned int set_psr_flag(unsigned int flag, bool on) {
	unsigned int old_psr = USLOSS_PsrGet();
	unsigned int new_psr;
	if (on) {
		new_psr = old_psr | flag;
	} else {
		new_psr = old_psr & ~flag;
	}
	int set_result = USLOSS_PsrSet(new_psr);
	if (set_result == -1) {
		return set_result; // ignore, getting rid of warnings
	}
	return old_psr;
}

void func_wrapper(void) {
	int returncode;
	void *arg = currentProcess->arg;
	if (arg != NULL) {
		returncode = currentProcess->startFunc(currentProcess->arg);
	} else {
		returncode = currentProcess->startFunc(NULL);
	}
	quit_phase_1a(returncode, -1); // should not run in 1a... maybe look through discord
}

void switch_context(unsigned int old_pid, int status, unsigned int new_pid) {
	currentProcess->status = status;
	// insert at tail of run queue
	currentProcess = processTable[new_pid];
	currentProcess->status = RUNNING;
	USLOSS_ContextSwitch(processTable[old_pid]->context, currentProcess->context);
}

=======
void TEMP_switchTo(int pid) {
	int psr = USLOSS_PsrGet();
	
	// Disable interrupts
	if (USLOSS_PsrSet(psr & ~USLOSS_PSR_CURRENT_INT) != USLOSS_DEV_OK) {
	    	USLOSS_Console("ERROR: USLOSS_PsrSet() failed in TEMP_switchTo.\n");
		USLOSS_Halt(1);
	}

	// Validate the PID
	if (pid < 0 || pid >= MAXPROC || processTable[pid].status == UNUSED) {
		USLOSS_Console("Error: Temp_switchTo()");
		USLOSS_Halt(1);
	}

	// Get the current and next process structs
	int current_pid = getpid();
	struct Process *current = &processTable[current_pid];
	struct Process *next = &processTable[pid];

	// Check if next process context is initialized
	if (next->context.start == NULL) {
		USLOSS_Console("Error: Attempted to switch to an uninitialized process context (PID %d).\n", pid);
		USLOSS_Halt(1);
	}

	// Print Debug info
	USLOSS_Console("TEMP_switchTo: Switching from PID %d to PID %d.\n", current_pid, pid);

	// Update current pid before switching
	currentPid = pid;

	// Save the current process's context if it is still valid
	if (current->status != TERMINATED) {
		USLOSS_ContextSwitch(&current->context, &next->context);
	} else {
		// If the current process is terminated, just switch
		USLOSS_ContextSwitch(NULL, &next->context);
	}

	// Re-enable interrupts
	if (USLOSS_PsrSet(psr) != USLOSS_DEV_OK) {
	    	USLOSS_Console("ERROR: USLOSS_PsrSet() failed in TEMP_switchTo.\n");
	    	USLOSS_Halt(1);
	}
}

void quit_phase_1a(int status, int switchToPid) {
	// disable interrupts
	int psr = USLOSS_PsrGet();

	// Make sure it is running in kernel mode
	if ((psr & USLOSS_PSR_CURRENT_MODE) == 0) {
		USLOSS_Console("Error: quit_phase_1a() called from user mode.\n");
		USLOSS_Halt(1);
	}
	
	// Disable interrupts
	if (USLOSS_PsrSet(psr & ~USLOSS_PSR_CURRENT_INT) != USLOSS_DEV_OK) {
    		USLOSS_Console("ERROR: USLOSS_PsrSet() failed in quit_phase_1a.\n");
    		USLOSS_Halt(1);
	}

	int pid = getpid();
	struct Process *current = &processTable[pid];

	// Check if the process has any children that are not joined
	if (current->first_child != NULL) {
		USLOSS_Console("Error: Process called quit_phase_1a() with unjoined children", pid);
		USLOSS_Halt(1);
	}

	// Mark the process TERMINATED and store exit status
	current->status = TERMINATED;
	current->exit_status = status;

	// Print debug info
	USLOSS_Console("Process %d has quit with status %d. Switching to process %d.\n", pid, status, switchToPid);

	// If a parent is waiting in join(), return the status
	if (current->parent && current->parent->waiting_for_child) {
		current->parent->waiting_for_child = 0;
	}

	// Context swtich to switchToPid
	TEMP_switchTo(switchToPid);

	USLOSS_Console("ERROR: TEMP_switchTo(%d) did not properly switch.\n", switchToPid);
    	USLOSS_Halt(1);
	
	while(1);
}

void dumpProcesses(void) {
	printf("dumpProcesses() called (not yet implemented)\n");

	for (int i = 0;i < MAXPROC; i++) {
		if (processTable[i].pid != -1) {
			printf("PID: %d, Name: %s, Status: %d, Priority: %d\n",
                   processTable[i].pid, processTable[i].name,
                   processTable[i].status, processTable[i].priority);
		}
	}	
}

void test_init() {
	phase1_init();

	// Check if init process is correctly initialized
	if (processTable[1].pid == 1 && strcmp(processTable[1].name, "init") == 0 &&
		processTable[1].priority == 6 && processTable[1].status == READY) {
		
		printf("init() test PASSED");
	} else {
		printf("init() test FAILED");	
	}
}

int child_process(void *arg) {
	printf("Child process %d running!\n", getpid());
	return 0;
}

void test_spork() {
	phase1_init();

	int pid = spork("child1", child_process, NULL, STACK_SIZE, 3);
	if (pid > 0 && processTable[pid].pid == pid &&
		strcmp(processTable[pid].name, "child1") == 0 &&
		processTable[pid].priority == 3) {
		
		printf("spork() test PASSED");
	} else {
		printf("spork() test FAILED");
	}
}

int child_process_exit(void *arg) {
	printf("Child process exiting!\n");
	quit(42);
	return 0; // should never reach here
}

void test_join() {
	phase1_init();

	int pid = spork("child", child_process_exit, NULL, STACK_SIZE, 3);
	int status;

	int joined_pid = join(&status);

	if (joined_pid == pid && status == 42) {
		printf("join() test PASSED");
	} else {
		printf("join() test FAILED (joined_pid: %d, status: %d", joined_pid, status);
	}
}
>>>>>>> Stashed changes

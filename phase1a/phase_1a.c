#include "phase1.h"

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
}

// Creates a new process, which is a child of the currently running process
// - creates an entry in the process table and fills it before calling dispatcher
// - if child is higher priority than parent, child will run before spork() returns
int spork(char *name, int (*startFunc)(void *), void *arg, int stackSize, int priority) {
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

// Phase1a implementation of quit() in 1b. Terminates the current process.
// Takes 2 parameters
// - status: exit status of this process. It will be returned to the parent through join()
// - switchToPid: tells us which process to switch to after we clean up the process
extern void quit_phase_1a(int status, int switchToPid) {
	int pidToTerminate = PID;

	// Get the current process so you can mark it TERMINATED
	Process *currProcess = &processTable[pidToTerminate % MAXPROC];
	curProcess->status = TERMINATED;
	
	// Should we halt if init process is trying to be terminated?
	
	// CONTEXT SWITCH TO NEXT PROCESS HERE
	TEMP_switchTo(switchToPid);

	// MAKE SURE THIS FUNCTION NEVER RETURNS
}

// Returns the current PID
extern int  getpid(void) {
	return PID;
}

// Prints summary of processes in the process table (pid, name, status, priority)
extern void dumpProcesses(void) {
	for (int i = 0; i < MAXPROC; i++) {
		USLOSS_Console("PID: %d, Name: %s, Status: %d, Priority: %d\n",
				processTable[i].pid, processTable[i].name,
				processTable[i].status, processTable[i].priority);
	}
}

// Calls USLOSS_ContextSwitch() to manually switch to the given process and
// save the old process state
void TEMP_switchTo(int pid) {
	// Check if we are in kernel mode (NEED TO DO THIS IN EVERY REQUIRED FUNCTION)
	if (USLOSS_PsrGet() != 1) {
		USLOSS_Console("Error: Cannot call TEMP_switchTo() in user mode\n");
		USLOSS_Halt(1);
	}	
	
	// Check if newpid is valid
	if (newpid < 0 || newpid >= MAXPROC || processTable[newpid].status == UNUSED) {
		USLOSS_Console("Error: Invalid PID\n");
		USLOSS_Halt(1);
	}

	// If everything checks out, continue to context switch
	int oldPid = getpid();  // get pid that we are switching from
	Process *oldProcess = &processTable[oldPid % MAXPROC];
	Process *newProcess = &processTable[newpid % MAXPROC];

	// UPDATE GLOBAL PID HERE
	
	// Context switch to new process and save old process state
	USLOSS_ContextSwitch(&oldProcess->context, &newProcess->context);
	
	// Code should never get here
	USLOSS_Console("Error: TEMP_switchTo() returned unexpectedly after context switch\n");
	USLOSS_Halt(1);
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

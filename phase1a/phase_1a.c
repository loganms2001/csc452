#include "phase1.h"

Process processTable[MAXPROC];
Process *currentProcess = NULL;
int availableProcSlots = MAXPROC;
int PID = 1;
bool kernel = true;

void phase1_init(void) {
	if (!kernel) {
		USLOSS_Console("Error: not in kernel mode.\n");
		USLOSS_Halt(1);
	}
	unsigned int oldPsr = set_psr_flag(USLOSS_PSR_CURRENT_INT, false);

	// initialize all processes in process table with pid
	for (int i = 0; i < MAXPROC; i++) {
		processTable[i].pid = PID_UNUSED;
		processTable[i].status = AVAILABLE;
	}
	
	Process *init = &processTable[1];

	// Initialize init process (PID 1)
	int stackSize = USLOSS_MIN_STACK;
	void *stack = malloc(stackSize);

	USLOSS_PTE *pageTable = phase5_mmu_pageTable_alloc(PID);
	USLOSS_ContextInit(&init->context, stack, stackSize, pageTable, func_wrapper);

	strncpy(init->name, "init", MAXNAME);
	init->pid = PID;
	init->priority = 6; // priority for init
	init->status = READY;
	init->exitStatus = -99;
	init->stackSize = stackSize;
	init->stack = stack;
	init->startFunc = init_entry;
	init->arg = NULL;

	// context defined above

	init->parent = NULL;
	init->firstChild = NULL;
	init->nextSibling = NULL;
	init->nextRun = NULL;

	USLOSS_PsrSet(oldPsr);

	availableProcSlots--;
}

// Creates a new process, which is a child of the currently running process
// - creates an entry in the process table and fills it before calling dispatcher
// - if child is higher priority than parent, child will run before spork() returns
int spork(char *name, int (*startFunc)(void *), void *arg, int stackSize, int priority) {
	unsigned int oldPsr = set_psr_flag(USLOSS_PSR_CURRENT_INT, false); // disables ints

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
	while (processTable[PID % MAXPROC].status != AVAILABLE) {
		PID++;
	}

	Process *newProc = &processTable[PID % MAXPROC];

	// complete process table entry
	void *stack = malloc(stackSize);
	USLOSS_PTE *pageTable = phase5_mmu_pageTable_alloc(PID);
	USLOSS_ContextInit(&newProc->context, stack, stackSize, pageTable, func_wrapper);

	strncpy(newProc->name, name, MAXNAME);
	newProc->pid = PID;
	newProc->priority = priority;
	newProc->status = READY;
	newProc->exitStatus = -99;
	newProc->stackSize = stackSize;
	newProc->stack = stack;
	newProc->startFunc = startFunc;
	newProc->arg = arg;

	// context defined above

	newProc->parent = currentProcess;
	newProc->firstChild = NULL;
	// nextSibling is defined below
	// nextRun is defined below
	
	availableProcSlots--;
	
	// inserts at head
	Process *temp = currentProcess->firstChild;
	currentProcess->firstChild = &processTable[PID % MAXPROC];
	currentProcess->firstChild->nextSibling = temp;
	
	// inserts at tail
	// processTable[PID%MAXPROC].nextSibling = NULL;
	// Process child = currentProcess->firstChild;
	// while (child != NULL) {
		// 	child = child->nextSibling;
		// }
		// child->nextSibling = processTable[PID%MAXPROC];
	
	// Process *cur = NULL;
	// Process *next = currentProcess;
	// while (next != NULL && next->priority >= priority) {
	// 	cur = next;
	// 	next = next->nextRun;
	// }
	
	// if (cur != NULL) {
	// 	cur->nextRun = &processTable[PID % MAXPROC];
	// }
	// cur->nextRun->nextRun = next;
	
	// call dispatcher in 1b
	// this would be responsible for disabling kernel mode

	USLOSS_PsrSet(oldPsr); // restore psr

	return PID;  // return new process pid
}

// Blocks the current process, until one of its children terminates and recieves its exit status.
// - (Phase 1a) In this phase, join() does not block. It returns if no terminated children are found
// 	- also returns if parent has no children 
int join(int *status) {
	unsigned int oldPsr = set_psr_flag(USLOSS_PSR_CURRENT_INT, false);

	// check for invalid status
	if (status == NULL) {
		return -3;
	}

	// check for no children
	if (currentProcess->firstChild == NULL) {
		return -2;
	}

	// Process runsBefore = NULL; // will have to deal with this later
	Process *prevChild = NULL;
	Process *curChild = currentProcess->firstChild;
	while (curChild->nextSibling != NULL && curChild->status != TERMINATED) {
		prevChild = curChild;
		curChild = curChild->nextSibling;
	}
	// if (curChild == NULL) blockMe();
	
	if (prevChild == NULL) {
		currentProcess->firstChild = curChild->nextSibling;
	} else {
		prevChild->nextSibling = curChild->nextSibling;
	}
	// Process childNextRun = curChild->nextRun; // later
	
	free(curChild->stack);

	curChild->status = AVAILABLE;

	availableProcSlots++;

	*status = curChild->exitStatus;

	USLOSS_PsrSet(oldPsr);

	return curChild->pid;
}

extern void quit_phase_1a(int status, int switchToPid) {
	if (!kernel) {
		USLOSS_Console("Error: not in kernel mode.\n");
		USLOSS_Halt(1);
	}
	currentProcess->exitStatus = status;
	switch_context(currentProcess->pid, TERMINATED, switchToPid);
}

extern int getpid(void) {
	return currentProcess->pid;
}

extern void dumpProcesses(void) {
	Process *p = currentProcess; // to make it shorter
	int sibPid;
	if (p->nextSibling != NULL) {
		sibPid = p->nextSibling->pid;
	} else {
		sibPid = -99;
	}
	int childPid;
	if (p->firstChild != NULL) {
		childPid = p->firstChild->pid;
	} else {
		childPid = -99;
	}
	USLOSS_Console("Name: %s\nPID: %d\nP-PID: %d\nS-PID: %d\nC-PID: %d\nPriority: %d\n Status: %d\n", 
		p->name, p->pid, p->parent->pid, sibPid, childPid, p->priority, p->status);
}

void TEMP_switchTo(int pid) {
	switch_context(PID, READY, pid);
}

////////////////  HELPERS  ////////////////

int init_entry(void *) {
	kernel = true;

	int newPid = spork("testcase_main", testcase_main, NULL, USLOSS_MIN_STACK, 3);
	switch_context(currentProcess->pid, READY, newPid);
	int status = 0;
	while (true) {
		if (join(&status) == -2) {
			if (status != 0) {
				USLOSS_Console("Error: exited with status %d\n", status);
			}
			USLOSS_Halt(status);
		}
	}
	return -99; // doesn't run
}

// USLOSS_PSR_CURRENT_MODE
// USLOSS_PSR_CURRENT_INT
unsigned int set_psr_flag(unsigned int flag, bool on) {
	unsigned int oldPsr = USLOSS_PsrGet();
	unsigned int newPsr;
	if (on) {
		newPsr = oldPsr | flag;
	} else {
		newPsr = oldPsr & ~flag;
	}
	USLOSS_PsrSet(newPsr);
	return oldPsr;
}

void func_wrapper(void) {
	kernel = false;
	int returncode = currentProcess->startFunc(currentProcess->arg);
	kernel = true;
	quit_phase_1a(returncode, currentProcess->parent->pid);
}

void switch_context(int old_pid, int status, int newPid) {
	unsigned int oldPsr = set_psr_flag(USLOSS_PSR_CURRENT_INT, false); // disable ints

	USLOSS_Context *old_context;
	if (currentProcess != NULL) {
		old_context = &currentProcess->context;
		currentProcess->status = status;
	} else {
		old_context = NULL;
	}
	// insert at tail of run queue
	currentProcess = &processTable[newPid % MAXPROC];
	currentProcess->status = RUNNING;

	USLOSS_PsrSet(oldPsr); // restore before switching contexts
	USLOSS_ContextSwitch(old_context, &currentProcess->context);
}
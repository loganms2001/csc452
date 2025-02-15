#include "phase1.h"

Process processTable[MAXPROC];
Process *currentProcess = NULL;
int availableProcSlots = MAXPROC;
int PID = 1;
// bool kernel = true;

void phase1_init(void) {
	enter_kernel_mode();
	unsigned int oldPsr = disable_interrupts();

	// initialize all processes in process table with pid
	for (int i = 0; i < MAXPROC; i++) {
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
	init->exitStatus = 0xBEEF;
	init->stackSize = stackSize;
	init->stack = stack;
	init->startFunc = init_entry;
	init->arg = NULL;

	// context defined above

	init->parent = NULL;
	init->firstChild = NULL;
	init->nextSibling = NULL;
	init->nextRun = NULL;

	restore_psr_state(oldPsr);

	availableProcSlots--;
}

// Creates a new process, which is a child of the currently running process
// - creates an entry in the process table and fills it before calling dispatcher
// - if child is higher priority than parent, child will run before spork() returns
int spork(char *name, int (*startFunc)(void *), void *arg, int stackSize, int priority) {
	check_kernel_true(__func__);
	unsigned int oldPsr = disable_interrupts();

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
	newProc->exitStatus = 0xBEEF;
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

	restore_psr_state(oldPsr);

	return PID;  // return new process pid
}

// Blocks the current process, until one of its children terminates and recieves its exit status.
// - (Phase 1a) In this phase, join() does not block. It returns if no terminated children are found
// 	- also returns if parent has no children 
int join(int *status) {
	check_kernel_true(__func__);
	unsigned int oldPsr = disable_interrupts();

	// check for invalid status
	if (status == NULL) {
		return -3;
	}

	// check for no children
	if (check_no_children()) {
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

	restore_psr_state(oldPsr);

	return curChild->pid;
}

extern void quit_phase_1a(int status, int switchToPid) {
	check_kernel_true(__func__);
	if (!check_no_children()) {
		USLOSS_Console("ERROR: cannot proceed with living children.\n");
		USLOSS_Halt(1);
	}
	currentProcess->exitStatus = status;
	switch_context(currentProcess->pid, TERMINATED, switchToPid);
}

extern int getpid(void) {
	return currentProcess->pid;
}

// EXAMPLE: 
// PID  PPID  NAME              PRIORITY  STATE
// 1     0  init              6         Runnable
// 2     1  testcase_main     3         Running
// 3     2  XXp1              2         Terminated(3)
// 4     2  XXp1              2         Terminated(4)
extern void dumpProcesses(void) {
	char state[32];
	char name_indent[32];
	int ind_len;
	USLOSS_Console(" PID  PPID  NAME              PRIORITY  STATE\n");
	for (int i = 0; i < MAXPROC; i++) {
		Process p = processTable[i];
		if (p.status == AVAILABLE) continue;
		ind_len = 18 - strlen(p.name);
		memset(name_indent, ' ', ind_len);
		name_indent[ind_len] = '\0';
		switch (p.status)
		{
		case READY:
			strncpy(state, "Runnable", 16);
		break;

		case RUNNING:
			strncpy(state, "Running", 16);
		break;

		case TERMINATED:
			sprintf(state, "Terminated(%d)", processTable[i].exitStatus);
		break;

		default:
			strncpy(state, "UNUSED", 16);
			break;
		}
		int ppid;
		if (p.parent == NULL) {
			ppid = 0;
		} else {
			ppid = p.parent->pid;
		}
		char pid_ind[] = {'\0', '\0', '\0'};
		if (p.pid < 10) {
			pid_ind[0] = ' ';
		}
		if (p.pid < 1000) {
			pid_ind[1] = ' ';
		}
		char ppid_ind[] = {'\0', '\0', '\0'};
		if (ppid < 10) {
			ppid_ind[0] = ' ';
		}
		if (ppid < 1000) {
			ppid_ind[1] = ' ';
		}
		USLOSS_Console(" %s%d     %d%s %s%s%d         %s\n",
			pid_ind, p.pid, ppid, ppid_ind, p.name, name_indent, p.priority, state);
	}
}

void TEMP_switchTo(int pid) {
	check_kernel_true(__func__);
	switch_context(PID, READY, pid);
}

////////////////  HELPERS  ////////////////

int init_entry(void *) {
	enter_kernel_mode();
	phase2_start_service_processes();
	phase3_start_service_processes();
	phase4_start_service_processes();
	phase5_start_service_processes();
	int newPid = spork("testcase_main", testcase_main, NULL, USLOSS_MIN_STACK, 3);
	USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to testcase_main() after using spork() to create it.\n");
	switch_context(currentProcess->pid, READY, newPid);
	// dispatcher called in spork
	int status = 0;
	while (true) {
		if (join(&status) == -2) {
			if (status != 0) {
				USLOSS_Console("ERROR: exited with status %d\n", status);
			}
			USLOSS_Halt(status);
		}
	}
	return -99; // doesn't run
}

void func_wrapper(void) {
	unsigned int oldPsr = enable_interrupts();
	int returncode = currentProcess->startFunc(currentProcess->arg);
	restore_psr_state(oldPsr);
	USLOSS_Console("Phase 1A TEMPORARY HACK: %s() returned, simulation will now halt.\n", currentProcess->name);
	quit_phase_1a(returncode, currentProcess->parent->pid);
}

void switch_context(int old_pid, int status, int newPid) {
	check_kernel_true(__func__);
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

	USLOSS_ContextSwitch(old_context, &currentProcess->context);
}

unsigned int disable_interrupts() {
	return set_psr_flag(USLOSS_PSR_CURRENT_INT, false);
}

unsigned int enable_interrupts() {
	return set_psr_flag(USLOSS_PSR_CURRENT_INT, true);
}

void restore_psr_state(unsigned int oldPsr) {
	int result = USLOSS_PsrSet(oldPsr);
	if (result != USLOSS_DEV_OK) {
		USLOSS_Console("PsrSet returned %d.\n", result);
		USLOSS_Halt(1);
	}
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
	int result = USLOSS_PsrSet(newPsr);
	if (result != USLOSS_DEV_OK) {
		USLOSS_Console("PsrSet returned %d.\n", result);
		USLOSS_Halt(1);
	}
	return oldPsr;
}

bool check_no_children() {
	return (currentProcess->firstChild == NULL);
}

void check_kernel_true(const char *func_name) {
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
		USLOSS_Console("ERROR: Someone attempted to call %s while in user mode!\n", func_name);
		USLOSS_Halt(1);
	}
}

void enter_kernel_mode() {
	set_psr_flag(USLOSS_PSR_CURRENT_MODE, true);
}
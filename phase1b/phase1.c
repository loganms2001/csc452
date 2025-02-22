/*
* Authors: Logan Sandlin and Christopher Le
* Phase 1-b
*
*/

#include "logchow.h"

Process processTable[MAXPROC];
Process *currentProcess = NULL;
int availableProcSlots = MAXPROC;
int PID = INIT_PID;
bool no_dispatch = false;

/// @brief Initial invocation is here. Creates init process.
void phase1_init() {
	enter_kernel_mode();
	unsigned int oldPsr = disable_interrupts();

	// initialize all processes in process table with pid
	for (int i = 0; i < MAXPROC; i++) {
		processTable[i].status = AVAILABLE;
	}
	
	Process *init = &processTable[PID];

	// Initialize init process (PID 1)
	int stackSize = USLOSS_MIN_STACK;
	void *stack = malloc(stackSize);

	USLOSS_PTE *pageTable = phase5_mmu_pageTable_alloc(PID);
	USLOSS_ContextInit(&init->context, stack, stackSize, pageTable, func_wrapper);

	strncpy(init->name, "init", MAXNAME);
	init->pid = PID;
	init->priority = 6; // priority for init
	init->status = READY;
	init->blockStatus = BLOCK_ON_COMMAND;
	init->exitStatus = 0xBEEF;
	init->stackSize = stackSize;
	init->stack = stack;
	init->startFunc = init_entry;
	init->arg = NULL;
	init->zapList[0] = 0;
	init->startTime = -1;

	// context defined above

	init->parent = NULL;
	init->firstChild = NULL;
	init->nextSibling = NULL;
	init->nextReady = NULL;

	restore_psr_state(oldPsr);

	availableProcSlots--;
}


/// @brief Creates a new process, which is the child of the current running process, 
///			then calls dispatcher (not yet though).
/// @param name Name of new process
/// @param startFunc Entry point of new process
/// @param arg Arg for new process entry
/// @param stackSize Size of stack fo process
/// @param priority Priority 1-5 of new process
/// @return PID of new process
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
	do {
		PID++;
	} while (processTable[PID % MAXPROC].status != AVAILABLE);

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
	newProc->zapList[0] = 0;
	newProc->startTime = -1;

	// context defined above

	newProc->parent = currentProcess;
	newProc->firstChild = NULL;

	// nextReady is set in here
	insert_priority_tail(currentProcess, newProc);
	
	// nextSibling is defined below
	
	// inserts at head
	Process *temp = currentProcess->firstChild;
	currentProcess->firstChild = &processTable[PID % MAXPROC];
	currentProcess->firstChild->nextSibling = temp;
	
	availableProcSlots--;

	dispatcher();

	restore_psr_state(oldPsr);

	return newProc->pid;  // return new process pid
}

/// @brief Waits for a child to terminate and cleans up the child (doesn't block yet).
/// @param status Out pointer to exit status
/// @return Dead child's pid
///			if error:
///				-3 for invalid out pointer
///				-2 for no children
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

	// may not be successful
	Process *prevChild = NULL;
	Process *curChild = currentProcess->firstChild;
	while (curChild->nextSibling != NULL && curChild->status != TERMINATED) {
		prevChild = curChild;
		curChild = curChild->nextSibling;
	}

	if (curChild->status != TERMINATED) {
		blockMe_wrapper(BLOCK_ON_CHILD);
	}

	// will certainly be successful
	prevChild = NULL;
	curChild = currentProcess->firstChild;
	while (curChild->nextSibling != NULL && curChild->status != TERMINATED) {
		prevChild = curChild;
		curChild = curChild->nextSibling;
	}
	
	if (prevChild == NULL) {
		currentProcess->firstChild = curChild->nextSibling;
	} else {
		prevChild->nextSibling = curChild->nextSibling;
	}
	
	free(curChild->stack);
	curChild->status = AVAILABLE;
	availableProcSlots++;
	*status = curChild->exitStatus;

	restore_psr_state(oldPsr);

	return curChild->pid;
}

/// @brief Exits current running process / changes status to terminated 
///			and calls dispatcher.
/// @param status Status to exit process with
extern void quit(int status) {
	check_kernel_true(__func__);
	if (!check_no_children()) {
		USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", currentProcess->pid);
		USLOSS_Halt(1);
	}
	currentProcess->exitStatus = status;
	currentProcess->status = TERMINATED;
	
	no_dispatch = true;

	int lenZapList = currentProcess->zapList[0];
	for (int i=lenZapList; i > 0; i--) { // zap stack instead of queue, don't like it tbh
		unblockProc(currentProcess->zapList[i]);
	}
	currentProcess->zapList[0] = 0;

	no_dispatch = false;

	unblockProc(currentProcess->parent->pid);

	dispatcher();

	// should never context switch back to here
	USLOSS_Console("ERROR: Process pid %d called quit() and came back to life.\n", currentProcess->pid);
	USLOSS_Halt(1);

	// gets rid of a warning
	exit(1);
}

/// @brief Returns PID of current process.
/// @return Current PID
extern int getpid() {
	return currentProcess->pid;
}


// EXAMPLE: 
// PID  PPID  NAME              PRIORITY  STATE
// 1     0  init              6         Runnable
// 2     1  testcase_main     3         Running
// 3     2  XXp1              2         Terminated(3)
// 4     2  XXp1              2         Terminated(4)

/// @brief Prints current active processtable
extern void dumpProcesses() {
	char state[64];
	char name_indent[32];
	char block_status[32];
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

		case BLOCKED:
			switch (p.blockStatus)
			{
			case BLOCK_ON_CHILD:
				strncpy(block_status, "waiting for child to quit", 32);
			break;

			case BLOCK_ON_ZAP:
				strncpy(block_status, "waiting for zap target to quit", 32);
			break;
			
			default:
				block_status[0] = '3';
				block_status[1] = '\0';
			break;
			}
			snprintf(state, 64, "Blocked(%s)", block_status);
		break;

		case TERMINATED:
			snprintf(state, 32, "Terminated(%d)", processTable[i].exitStatus);
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
		// if (p.nextReady == NULL) {
		// 	ppid = 0;
		// } else {
		// 	ppid = p.nextReady->pid;
		// }
		char pid_ind[] = {'\0', '\0', '\0'};
		if (p.pid < 100) {
			pid_ind[0] = ' ';
		}
		if (p.pid < 10) {
			pid_ind[1] = ' ';
		}
		char ppid_ind[] = {'\0', '\0', '\0'};
		if (ppid < 100) {
			ppid_ind[0] = ' ';
		}
		if (ppid < 10) {
			ppid_ind[1] = ' ';
		}
		USLOSS_Console(" %s%d   %s%d  %s%s%d         %s\n",
			pid_ind, p.pid, ppid_ind, ppid, p.name, name_indent, p.priority, state);
	}
}

/// @brief Switches context to the next scheduled context
void dispatcher() {
	// // debug
	// USLOSS_Console("Dispatcher called.\n");
	Process *nextProcess = NULL;
	if (currentProcess == NULL) {
		nextProcess = &processTable[INIT_PID];
	} else if (currentProcess->status == TERMINATED || currentProcess->status == BLOCKED) {
		nextProcess = currentProcess->nextReady;
	} else {
		if (currentProcess->nextReady != NULL && (currentProcess->nextReady->priority < currentProcess->priority // normal switch
			|| (currentProcess->nextReady->priority == currentProcess->priority && quantum_expired()))) { // clock switch
			nextProcess = currentProcess->nextReady;
			// nextProcess->nextReady set in here
			insert_priority_tail(nextProcess, currentProcess);
		}
	}
	if (nextProcess != NULL) {
		switch_context(nextProcess);
	}
}

void blockMe_wrapper(int status) {
	currentProcess->blockStatus = status;
	blockMe();
}

void blockMe() {
	currentProcess->status = BLOCKED;
	dispatcher();
}

int unblockProc(int pid) {
	Process *proc = &processTable[pid % MAXPROC];
	if (proc->status == BLOCKED) {
		proc->status = READY;
		currentProcess->blockStatus = BLOCK_ON_COMMAND;
		insert_priority_tail(currentProcess, proc);
		if (!no_dispatch) dispatcher();
		return 0;
	}
	return -2;
}

void zap(int pid) {
	Process *proc = &processTable[pid % MAXPROC];
	// error check for: zap self
	if (proc == currentProcess) {
		USLOSS_Console("ERROR: Attempt to zap() itself.\n");
		USLOSS_Halt(1);
	}
	// error check for: proc nonexist
	if (proc->pid != pid || proc->status == AVAILABLE) {
		USLOSS_Console("ERROR: Attempt to zap() a non-existent process.\n");
		USLOSS_Halt(1);
	}
	// error check for: proc dead
	if (proc->status == TERMINATED) {
		USLOSS_Console("ERROR: Attempt to zap() a process that is already in the process of dying.\n");
		USLOSS_Halt(1);
	}
	// error check for: proc init
	if (pid == INIT_PID) {
		USLOSS_Console("ERROR: Attempt to zap() init.\n");
		USLOSS_Halt(1);
	}

	int curZapLen = proc->zapList[0];
	proc->zapList[curZapLen+1] = currentProcess->pid;
	proc->zapList[0] = curZapLen+1;
	blockMe_wrapper(BLOCK_ON_ZAP);
}

////////////////  HELPERS  ////////////////

bool quantum_expired() {
	double ms_elapsed = ((double)(clock() - currentProcess->startTime) / CLOCKS_PER_SEC) * 1000;
	bool expired = ms_elapsed >= QUANTUM_MS;
	return expired;
}

void insert_priority_tail(Process *front, Process *proc) {
	if (proc->pid == INIT_PID) {
		proc->nextReady = NULL;
	}

	Process *newHead = front->nextReady;
	if (newHead == NULL) {
		newHead = proc;
		proc->nextReady = front;
	} else if (proc->priority < newHead->priority) {
		proc->nextReady = newHead;
		newHead = proc;
	} else {
		Process *cur = newHead;
		while (cur->nextReady != NULL && cur->nextReady->priority <= proc->priority) {
			cur = cur->nextReady;
		}
		Process *temp = cur->nextReady;
		cur->nextReady = proc;
		proc->nextReady = temp;
	}
	
	front->nextReady = newHead;
}

/// @brief Entry function for init process
/// @param  UNUSED No param is passed to init
/// @return Should never return
int init_entry(void *unused) {
	enter_kernel_mode();

	phase2_start_service_processes();
	phase3_start_service_processes();
	phase4_start_service_processes();
	phase5_start_service_processes();

	spork("testcase_main", (int (*)(void *)) testcase_main, NULL, USLOSS_MIN_STACK, 3);
	
	int status = 0;
	while (true) {
		if (join(&status) == -2) {
			if (status != 0) {
				USLOSS_Console("ERROR: exited with status %d\n", status);
			}
			USLOSS_Halt(status);
		}
	}

	// doesn't run
	USLOSS_Halt(1);
	return -99;
}

/// @brief Wrapper for catching function returns in functions called by USLOSS. Temp since 
///			testcase returns.
void func_wrapper() {
	unsigned int oldPsr = enable_interrupts();
	int returncode = currentProcess->startFunc(currentProcess->arg);
	restore_psr_state(oldPsr);
	quit(returncode);
}

/// @brief Switches contexts from old PID to new
/// @param old_pid Old PID
/// @param status Status to switch the old process to
/// @param newPid New PID
void switch_context(Process *newProcess) {
	check_kernel_true(__func__);
	USLOSS_Context *old_context;
	if (currentProcess != NULL) {
		old_context = &currentProcess->context;
		if (currentProcess->status == RUNNING) {
			currentProcess->status = READY;
		}
	} else {
		old_context = NULL;
	}
	// insert at tail of ready queue
	currentProcess = newProcess;
	currentProcess->status = RUNNING;
	currentProcess->startTime = clock();

	USLOSS_ContextSwitch(old_context, &currentProcess->context);
}

/// @brief Disables interrupts.
/// @return The old psr
unsigned int disable_interrupts() {
	return set_psr_flag(USLOSS_PSR_CURRENT_INT, false);
}

/// @brief Enables interrupts.
/// @return The old psr
unsigned int enable_interrupts() {
	return set_psr_flag(USLOSS_PSR_CURRENT_INT, true);
}

/// @brief Restores PSR.
/// @param oldPsr Previous PSR state
void restore_psr_state(unsigned int oldPsr) {
	int result = USLOSS_PsrSet(oldPsr);
	if (result != USLOSS_DEV_OK) {
		USLOSS_Console("PsrSet returned %d.\n", result);
		USLOSS_Halt(1);
	}
}

/// @brief Sets a given PSR flag to the bool given.
/// @param flag Flag to set
/// @param on On or off
/// @return Old PSR
unsigned int set_psr_flag(unsigned int flag, bool on) {
	unsigned int oldPsr = USLOSS_PsrGet();
	unsigned int newPsr;
	if (on) {
		newPsr = oldPsr | flag;
	} else {
		newPsr = oldPsr & ~flag;
	}
	// not REstoring, just storing
	restore_psr_state(newPsr);
	return oldPsr;
}

/// @brief Checks to make sure there are no children of current process
/// @return Bool of children
bool check_no_children() {
	return (currentProcess->firstChild == NULL);
}

/// @brief Exits with an error code if in user mode
/// @param func_name Name for error message
void check_kernel_true(const char *func_name) {
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
		USLOSS_Console("ERROR: Someone attempted to call %s while in user mode!\n", func_name);
		USLOSS_Halt(1);
	}
}

/// @brief Sets kernel mode flag true
void enter_kernel_mode() {
	set_psr_flag(USLOSS_PSR_CURRENT_MODE, true);
}
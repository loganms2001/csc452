#include "phase1.h"

Process processTable[MAXPROC];

int main() {
    struct Process a;
    phase1_init();
    puts("compiles and runs");
}

void phase1_init(void) {
	// Init process (PID 1)
	strcpy(processTable[1].name, "init");
	processTable[1].pid = 1;
	processTable[1].priority = 6;
	processTable[1].status = READY;
}

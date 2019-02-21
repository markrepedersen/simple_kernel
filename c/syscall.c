#include <xeroskernel.h>
#include <stdarg.h>

int syscall(int call, ...) {
	va_list args;
	va_start(args, call);
 	// set random value to check if functional
	int retVal = 999;

	__asm__ (
		"movl %1, %%eax;"
		"movl %2, %%edx;"
		"int %3;"
		"movl %%eax, %0;"
		: "=g" (retVal)
		: "g" (call), "g" (args), "i" (INTERRUPT_NUM)
		: "eax", "edx");

	if (retVal == 999) {
		kprintf("FAILURE: return value was not set correctly");
	}
	return retVal;
}

/**
* Gets the PID of a newly created process.
* Returns NULL if no process's available.
*/
static int getPID(void) {
    PCB *curr = readyQueue;
    while (curr) {
        if (curr->next == NULL) {
            return curr->pid;
        }
        curr = curr->next;
    }
    return NULL;
}

unsigned int syscreate(functionPointer func, int stack) {
    syscall(CREATE, func, stack);
    return getPID();
}

void sysyield(void) {
	syscall(YIELD);
}

void sysstop(void) {
	syscall(STOP);
}

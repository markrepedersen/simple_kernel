#include <xeroskernel.h>

#define SAFETY_MARGIN 16

/**
* Pop the first unused PCB from the stopped queue.
* Returns NULL if no free process control blocks.
*/
PCB *getPCB(void) {
	PCB *createdProcess = stoppedQueue;
	if (createdProcess != NULL) {
		stoppedQueue = stoppedQueue->next;
		return createdProcess;
	}
	return NULL;
}

/**
* Adds a PCB block to the tail of the ready queue.
* The PCB block is put into the lowest priority queue of the ready queue.
*/
int addReady(char *stackAddress, void *originalSP) {
	PCB *newProcess = getPCB();
	if (!newProcess) return 0;
	else {
		newProcess->esp = (unsigned long) stackAddress;
		newProcess->originalSp = (unsigned long) originalSP;
	}

	PCB *curr = readyQueue[LOW_PRIORITY];
	if (curr) {
		while (curr) {
			if (!curr->next) {
				curr->next = newProcess;
				newProcess->next = NULL;
				return newProcess->pid;
			}
			curr = curr->next;
		}
	}
	else readyQueue[LOW_PRIORITY] = newProcess;
	return 0;
}

/**
* Initialize the initial context of a process's stack.
*/
static void initContext(functionPointer func, char *stackAddress) {
	context_frame* initialContext = (context_frame*) stackAddress;
	initialContext->ebp = (unsigned long) initialContext;
	initialContext->esp = (unsigned long) initialContext;
	initialContext->ret_eip = (unsigned long) func;
	initialContext->iret_cs = getCS();
	initialContext->eflags = 0x00003200;

	// Add fallback return address.
	functionPointer* fallback = (functionPointer*) (initialContext + 1);
	*fallback = &sysstop;
}

/**
* Create a new process.
* Returns 0 if no process control blocks are available, and the process' id otherwise.
*/
int create(functionPointer func, int stack) {
	void *sp = kmalloc(stack);
	char *stackAddress = (char*) sp + stack - sizeof(context_frame) - sizeof(void*) - SAFETY_MARGIN;
	initContext(func, stackAddress);
	return addReady(stackAddress, sp);
}
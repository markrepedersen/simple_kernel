#include <xeroskernel.h>

#define SAFETY_MARGIN 16

/**
* Pop the first unused PCB from the stopped queue.
* Returns NULL if no free process control blocks.
*/
PCB* getPCB(void) {
	PCB *createdProcess = stoppedQueue;
	if (createdProcess != NULL) {
		stoppedQueue = stoppedQueue->next;
		return createdProcess;
	}
	return NULL;
}

/**
* Adds a PCB block to the tail of the ready queue.
*/
int addReady(char* stackAddress, void* originalSP) {
	PCB *newProcess = getPCB();
	if (!newProcess) return 0;
	newProcess->esp = (unsigned long) stackAddress;
	newProcess->originalSp = (unsigned long) originalSP;
	if (!readyQueue) readyQueue = newProcess;
	else {
		PCB *curr = readyQueue;
		while (curr) {
			if (!curr->next) {
				curr->next = newProcess;
				newProcess->next = NULL;
				return 1;
			}
			curr = curr->next;
		}
	}
	return 0;
}

/**
* Initialize the initial context of a process's stack.
*/
void initContext(functionPointer func, char *stackAddress) {
	context_frame context = { 0 };

	context.ebp = (unsigned long) stackAddress;
	context.esp = (unsigned long) stackAddress;
	context.ret_eip = (unsigned long) func;
	context.iret_cs = getCS();
	context.eflags = 0x00003000;

	*((context_frame*) stackAddress) = context;
}

/**
* Create a new process.
* Returns 0 if no process control blocks are available, and 1 otherwise.
*/
int create(functionPointer func, int stack) {
	void *sp = kmalloc(stack);
	char *stackAddress = (char*) sp + stack - sizeof(context_frame) - SAFETY_MARGIN;
	initContext(func, stackAddress);
	return addReady(stackAddress, sp);
}
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
int addReady(char* stackAddress, void* memoryStart, char* memoryEnd, int isIdle) {
	PCB *newProcess = getPCB();
	if (!newProcess) return 0;

	newProcess->esp = (unsigned long) stackAddress;
	newProcess->originalSp = (unsigned long) memoryStart;
	newProcess->memoryEnd = memoryEnd;
	newProcess->signalMask = 0;
	if (isIdle) {
		idleProcess = newProcess;
		return newProcess->pid;
	}

	addToBack(newProcess, &readyQueue[LOW_PRIORITY]);
	return newProcess->pid;
}

/**
* Initialize the context of a process's stack.
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
* If the function passed is NULL, then the idle process is created.
* Returns 0 if no process control blocks are available, and the process' id otherwise.
*/
int create(functionPointer func, int stack) {
	void *sp = kmalloc(stack);
	char *processMemoryEnd = (char*) sp + stack - sizeof(void*) - SAFETY_MARGIN;
	char *stackAddress = processMemoryEnd - sizeof(context_frame);
	initContext(func == NULL ? &idleproc : func, stackAddress);
	return addReady(stackAddress, sp, processMemoryEnd, func == NULL);
}
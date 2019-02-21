#include <xeroskernel.h>
#include <stdarg.h>

void addToStoppedQueue(PCB *pcb) {
	PCB *prevFront = stoppedQueue;
	stoppedQueue = (PCB*) pcb;
	stoppedQueue->next = prevFront;
}

/**
* Cleans up a process by freeing its allocated stack and putting the process on the stopped queue.
*/
void cleanup(PCB *pcb) {
	kfree((void*) pcb->originalSp); // we don't know esp is pointing to the low address of stack; it might've been incremented since being allocated
	pcb->esp = NULL;
	addToStoppedQueue(pcb);
}

/**
* Removes the next process from the ready queue and returns a pointer to its process control block
*/
PCB* next(void) {
	PCB *pop = (PCB*) readyQueue;
	if (pop != NULL) {
		readyQueue = (PCB*) pop->next;
	}
	return pop;
}

/**
* Takes a pointer to a process control block and adds it to the back of the ready queue
*/
void ready(PCB *pcb) {
	if (!readyQueue) {
		readyQueue = pcb;
		pcb->next = NULL;
		return;
	}
	PCB *curr = readyQueue;
	while (curr != NULL) {
		if (curr->next == NULL) {
			curr->next = pcb;
			pcb->next = NULL;
			return;
		}
		curr = (PCB*) curr->next;
	}
}

void dispatch() {
	PCB* process = next();
	for (;;) {
		int contextAddress = contextswitch(process);
		context_frame *context = (context_frame*) contextAddress;
		REQUEST_TYPE call = (REQUEST_TYPE) ((REQUEST_TYPE) context->eax);
		switch(call) {
			case CREATE: {
				va_list params = ((va_list) (*context).edx);
				functionPointer func = va_arg(params, functionPointer);
				int stackSize = (int) va_arg(params, int);
				create(func, stackSize);
				break;
			}
			case YIELD: {
				ready(process);
				process = next();
				break;
			}
			case STOP: {
				cleanup(process);
				process = next();
				break;
			}
		}
	}
}
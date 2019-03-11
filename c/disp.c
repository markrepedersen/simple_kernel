#include <xeroskernel.h>
#include <stdarg.h>
#include <i386.h>

PCB *nextHighestPriorityProcess(void) {
	PCB *pop = NULL;
	for (int i = 0; i < sizeof(readyQueue) / sizeof(readyQueue[0]); ++i) {
		pop = (PCB*) readyQueue[i];
		if (pop) {
			readyQueue[i] = (PCB*) pop->next;
			break;
		}
	}
	return pop;
}

void addToStoppedQueue(PCB *pcb) {
	PCB *prevFront = stoppedQueue;
	stoppedQueue = (PCB*) pcb;
	if (pcb != NULL) {
		pcb->next = NULL;
	}
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
* Remove the next highest priority process in the ready queue.
*/
PCB *next(void) {
	PCB *pop = nextHighestPriorityProcess();
	initPIT(100);
	return pop;
}

/**
* Adds a process to the back of the ready queue.
*/
void ready(PCB *pcb) {
	int priority_level = pcb->priority;
	PCB *curr = readyQueue[priority_level];
	if (!curr) {
		curr = pcb;
		curr->next = NULL;
		return;	
	}

	while (curr != NULL) {
		if (curr->next == NULL) {
			curr->next = pcb;
			pcb->next = NULL;
			return;
		}
		curr = (PCB*) curr->next;
	}
}

/**
 * Searches the ready queue for a process with the given pid.
 * If no such process is found, returns NULL.
 */
static PCB *findProcess(PID_t pid) {
	for (int i = 0; i < sizeof(readyQueue) / sizeof(readyQueue[0]); ++i) {
		PCB *curr = readyQueue[i];
		while (curr) {
			if (curr->pid == pid) return curr;
			curr = curr->next;
		}
	}
	return NULL;
}

void dispatch() {
	PCB *process = next();
	for (;;) {
		if (!process) {
			kprintf("Out of processes! Dying\n");
			for (;;);
		}
		context_frame *context = (context_frame*) contextswitch(process);
		REQUEST_TYPE call = (REQUEST_TYPE) context->eax;
		switch(call) {
			case CREATE: {
				va_list params = ((va_list) context->edx);
				functionPointer func = va_arg(params, functionPointer);
				int stackSize = (int) va_arg(params, int);
				process->ret = create(func, stackSize);
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
			case GET_PID: {
				process->ret = process->pid;
				break;
			}
			case PUT_STRING: {
				va_list params = ((va_list) context->edx);
				char* str = va_arg(params, char*);
				kprintf(str);
				break;
			}
			case KILL: {
				va_list params = ((va_list) context->edx);
				PID_t pid = va_arg(params, PID_t);
				PCB* targetProcess = findProcess(pid);
				if (!targetProcess) {
					process->ret = -1;
					kprintf("Process with id %d not found.\n", pid);
				} else {
					process->ret = 0;
					cleanup(targetProcess);
					if (targetProcess == process) process = next();
				}
				break;
			}
			case PRIORITY: {
				va_list params = ((va_list) context->edx);
				int targetPriority = va_arg(params, int);
				if (targetPriority >= -1 && targetPriority <= 3) {
					process->ret = process->priority;
					if (targetPriority != -1) process->priority = targetPriority;
				} else {
					kprintf("Bad priority requested: %d\n", targetPriority);
					process->ret = -1;
				}
				break;	
			}
			case TIMER_INT: {
				ready(process);
				process = next();
				end_of_intr();
				break;
			}
			default: {
				kprintf("Bad request %d in process %d\n", call, process->pid);
				kprintf("Stopping...\n");
				for (;;);
			}
		}
	}
}
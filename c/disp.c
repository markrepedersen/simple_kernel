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

/**
 * Searches the ready queue for a process with the given pid.
 * If no such process is found, returns NULL.
 */
static PCB* findProcess(PID_t pid) {
	PCB* curr = readyQueue;
	while (curr) {
		if (curr->pid == pid) return curr;
		curr = curr->next;
	}
	return NULL;
}

void dispatch() {
	PCB* process = next();
	for (;;) {
		if (!process) {
			kprintf("Out of processes! Dying\n");
			for (;;);
		}
		int contextAddress = contextswitch(process);
		context_frame *context = (context_frame*) contextAddress;
		REQUEST_TYPE call = (REQUEST_TYPE) ((REQUEST_TYPE) context->eax);
		switch(call) {
			case CREATE: {
				va_list params = ((va_list) (*context).edx);
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
				va_list params = ((va_list) (*context).edx);
				char* str = va_arg(params, char*);
				kprintf(str);
				break;
			}
			case KILL: {
				va_list params = ((va_list) (*context).edx);
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
				va_list params = ((va_list) (*context).edx);
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
			default: {
				kprintf("Bad request %d in process %d\n", call, process->pid);
				kprintf("Stopping...\n");
				for (;;);
			}
		}
	}
}
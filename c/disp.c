#include <xeroskernel.h>
#include <stdarg.h>
#include <i386.h>

PCB *nextHighestPriorityProcess(void) {
	PCB *pop = NULL;
	for (int i = 0; i < sizeof(readyQueue) / sizeof(readyQueue[0]); ++i) {
		pop = readyQueue[i];
		if (pop) {
			readyQueue[i] = pop->next;
			break;
		}
	}
	return pop;
}



/**
* Cleans up a process by freeing its allocated stack and putting the process on the stopped queue.
*/
void cleanup(PCB *pcb) {
	// Clear out senders
	PCB* curr = pcb->senders;
	while (curr) {
		PCB* next = curr->next;
		ready(curr);
		curr = next;
	}
	// Search receivers
	curr = blockedQueue;
	while (curr) {
		PCB* next = curr->next;
		if (*(curr->senderPID) == pcb->pid) {
			ready(curr);
		}
		curr = next;
	}
	pcb->senders = NULL;

	removeFromQueue(pcb, &(readyQueue[pcb->priority]));
	kfree((void*) pcb->originalSp); // we don't know esp is pointing to the low address of stack; it might've been incremented since being allocated
	pcb->esp = NULL;
	pcb->pid += PROCESS_TABLE_SIZE;
	addToBack(pcb, &stoppedQueue);
}

/**
* Remove the next highest priority process in the ready queue.
* If none exist, then run the idle process.
*/
PCB *next(void) {
	PCB *pop = nextHighestPriorityProcess();
	initPIT(TIME_SLICE * 10);
	return pop != NULL ? pop : idleProcess;
}

/**
* Adds a process to the back of the ready queue.
*/
void ready(PCB *pcb) {
	addToBack(pcb, &(readyQueue[pcb->priority]));
}

/**
 * Removes a process from a queue. Assumes the queue is valid.
 * Returns 1 if the queue was removed, 0 if it was not present.
 */
int removeFromQueue(PCB* pcb, PCB** queue) {
	if (!*queue) return 0;
	if (*queue == pcb) {
		*queue = pcb->next;
		return 1;
	}
	PCB* curr = *queue;
	while (curr) {
		if (curr->next == pcb) {
			curr->next = pcb->next;
			return 1;
		}
		curr = curr->next;
	}
	return 0;
}

/**
 * Adds a process to the back of a queue. Assumes the queue is valid.
 */
void addToBack(PCB* pcb, PCB** queue) {
	pcb->next = NULL;
	if (!*queue) {
		*queue = pcb;
	} else {
		PCB* curr = *queue;
		while(curr != NULL){
			if (curr->next == NULL) {
				curr->next = pcb;
				return;
			}
			curr = curr->next;
		}

	}
}

/**
 * Adds a process to the front of a queue. Assumes the queue is valid.
 */
void addToFront(PCB *pcb, PCB** queue) {
	pcb->next = *queue;
	*queue = pcb;
}

/**
 * Searches for a ready process with the given pid.
 * If no such process is found, returns NULL.
 */
PCB *findReadyProcess(PID_t pid) {
	for (int i = 0; i < sizeof(readyQueue) / sizeof(readyQueue[0]); ++i) {
		PCB* process = findProcess(pid, readyQueue[i]);
		if (process) return process;
	}
	return NULL;
}

PCB *findProcess(PID_t pid, PCB* queue) {
	PCB* curr = queue;
	while (curr) {
		if (curr->pid == pid) return curr;
		curr = curr->next;
	}
	return NULL;
}

void dispatch() {
	PCB *process = next();
	for (;;) {
		context_frame *context = (context_frame*) contextswitch(process);
		REQUEST_TYPE call = (REQUEST_TYPE) context->eax;
		va_list params = (va_list) context->edx;
		switch(call) {
			case CREATE: {
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
				char* str = va_arg(params, char*);
				kprintf(str);
				break;
			}
			case KILL: {
				PID_t pid = va_arg(params, PID_t);
				if (pid == process->pid) {
					cleanup(process);
					process = next();
				} else {
					PCB* targetProcess = findReadyProcess(pid);
					if (!targetProcess) {
						process->ret = -1;
						kprintf("Process with id %d not found.\n", pid);
					} else {
						process->ret = 0;
						cleanup(targetProcess);
					}
				}
				break;
			}
			case PRIORITY: {
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
				tick();
				ready(process);
				process = next();
				end_of_intr();
				break;
			}
			case SEND: {
				PID_t pid = va_arg(params, PID_t);
				int value = va_arg(params, int);
				if (!send(pid, process, value)) process = next();
				break;
			}
			case RECEIVE: {
				PID_t* pid = va_arg(params, PID_t*);
				unsigned int* num = va_arg(params, unsigned int*);
				process->recvLocation = num;
				if (!recv(pid, process)) process = next();
				break;
			}
			case SLEEP: {
				// kprintf("start of sleep\n");
				int ticks = va_arg(params, int);
				process->timeSlice = ticks;
				sleep(process);
				// kprintf("middle\n");
				process = next();
				process->ret = 0; // the amount of time the sleep has left once unblocked. In this assignment it returns 0 always.
				// kprintf("end of sleep\n");
				break;
			}
			default: {
				kprintf("Bad request %d in process %d\n", call, process->pid);
				kprintf("Stopping...\n");
				for (;;);
			}
		}
	}
	if (!process) {
		kprintf("Out of processes! Dying\n");
		for (;;);
	}
}
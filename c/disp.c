#include <xeroskernel.h>
#include <stdarg.h>
#include <i386.h>

extern char *maxaddr;        /* end of memory address range */

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
* Cleans up the signal handlers for the process when it has finished.
* When reused, other processes will have different signal handlers.
*/
void clearSignalTable(PCB *pcb) {
	signalHandler *signalTable = pcb->signalTable;
	for (int i = 0; i < sizeof(signalTable) / sizeof(signalTable); ++i) {
		signalTable[i] = NULL;
	}
	pcb->signalMask = 0;
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
	clearSignalTable(pcb);
	removeFromQueue(pcb, &(readyQueue[pcb->priority]));
	kfree((void*) pcb->originalSp); // we don't know esp is pointing to the low address of stack; it might've been incremented since being allocated
	pcb->esp = NULL;
	pcb->pid += PROCESS_TABLE_SIZE;
	addToBack(pcb, &stoppedQueue);

	PCB *waitingProcess = pcb->waitingProcess;
	if (waitingProcess) { // syswait
		ready(waitingProcess);
		waitingProcess = NULL;
	}
}

/**
* Remove the next highest priority process in the ready queue.
* If none exist, then run the idle process.
*/
PCB *next(void) {
	PCB *nextProcess = nextHighestPriorityProcess();
	if (nextProcess) {
		initPIT(1000 * 10);
		if (nextProcess->signalMask) { // process has a pending signal
			deliverSignals(nextProcess);
		}
	}
	return nextProcess ? nextProcess : idleProcess; // next process is the idle process if no other process to run
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

/**
* Looks for the process with the given pid in the given queue.
*/
PCB *findProcess(PID_t pid, PCB* queue) {
	PCB *curr = queue;
	while (curr) {
		if (curr->pid == pid) return curr;
		curr = curr->next;
	}
	return NULL;
}

/**
* Finds a process if it is not stopped and not the idle process.
*/
PCB *findExistingProcess(PID_t pid) {
	PCB *process = findProcess(pid, stoppedQueue);
	if (!process) { // process is not in stopped queue
		return &processTable[(pid % 32) - 1];
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
				int stackSize = va_arg(params, int);
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
				int ticks = va_arg(params, int);
				process->timeSlice = ticks;
				sleep(process);
				process = next();
				break;
			}
			case SIGNAL_HANDLER: {
					int signal = va_arg(params, int);
					signalHandler newHandler = va_arg(params, signalHandler);
					signalHandler *oldHandler = (signalHandler*) va_arg(params, signalHandler);

					if (signal < 0 || signal >= MAX_SIGNALS-1) {
						kprintf("Invalid signal.\n");
						process->ret = -1;
					} 
					else if (((unsigned long) newHandler >= HOLESTART && (unsigned long) newHandler <= HOLEEND) || (unsigned long) newHandler > (unsigned long) maxaddr || (unsigned long) newHandler < 0) { // invalid address for handler
						kprintf("New handler address is out of memory bounds.\n");
						process->ret = -2;
					}
					else if (((unsigned long) oldHandler >= HOLESTART && (unsigned long) oldHandler <= HOLEEND) || (unsigned long) oldHandler > (unsigned long) maxaddr || (unsigned long) oldHandler < 0) { // invalid address for handler
						kprintf("Old handler address is out of memory bounds.\n");
						process->ret = -3;
					}
					else if (!newHandler) {
						kprintf("New handler is NULL. Ignoring signal.\n");
						process->ret = 0;
					}
					else {
						kprintf("Setting signal handler for signal %d.\n", signal);
						*oldHandler = process->signalTable[signal]; // give back old handler to application
						process->signalTable[signal] = newHandler;
						process->ret = 0;
					}
					break;
			}
			case SIGNAL_RETURN: {
				unsigned long oldSP = va_arg(params, unsigned long);
				process->esp = (unsigned long) oldSP;
				signal_context *savedContext = (signal_context*) (oldSP - sizeof(signal_context));
				process->ret = savedContext->return_value;
				process->signalMask ^= 1UL << savedContext->signalNum;
				break;
			}
			case SIGNAL_WAIT: {
				PID_t pid = va_arg(params, PID_t);
				PCB *foundProcess = findProcess(pid, stoppedQueue);
				if (!foundProcess || pid == process->pid) {
					process->ret = -1;
				} else foundProcess->waitingProcess = process;
				break;
			}
			case SIGNAL_KILL: {
				PID_t pid = va_arg(params, PID_t);
				int signalNumber = va_arg(params, int);
				PCB *targetProcess = findExistingProcess(pid);

				if (!targetProcess) {
					kprintf("A signal was raised for process %d, but this process does not exist.\n", pid);
					process->ret = -514;
				}
				else if (signalNumber < 0 || signalNumber >= MAX_SIGNALS-1) {
					kprintf("Signal for process: %d is invalid.\n", pid);
					process->ret = -583;
				}
				else {
					signal(targetProcess, signalNumber);
					process->ret = 0;
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
	if (!process) {
		kprintf("Out of processes! Dying\n");
		for (;;);
	}
}
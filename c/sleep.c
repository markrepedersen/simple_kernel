#include <xeroskernel.h>
#include <xeroslib.h>

/* sleep.c : sleep device
   This file does not need to modified until assignment 2
 */

void insert(PCB *process) {
	PCB *curr = sleepQueue;
	PCB *prev = NULL;
	if (!curr) {
		sleepQueue = process;
		process->next = NULL;
		return;
	}
	while (curr) {
		// kprintf("curr: \n");
		if (curr->timeSlice <= process->timeSlice) {
			process->timeSlice = process->timeSlice - curr->timeSlice;
			if (!curr->next) {
				curr->next = process;
				process->next = NULL;
			}
		} else {
			PCB *temp = curr;
			if (prev) {
				prev->next = process;
			}
			process->next = temp;
		}
		prev = curr;
		curr = curr->next;
	}
}

void sleep(PCB *process) {
	insert(process);
}

void tick(void) {
	if (sleepQueue) {
		if (sleepQueue->timeSlice <= 0) {
			PCB *wokenUpProcess = sleepQueue;
			sleepQueue = sleepQueue->next;
			wokenUpProcess->next = NULL;
			ready(wokenUpProcess);
		}
		sleepQueue->timeSlice--;
	}
}
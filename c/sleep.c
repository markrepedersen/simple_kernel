#include <xeroskernel.h>
#include <xeroslib.h>

/* sleep.c : sleep device 
   This file does not need to modified until assignment 2
 */

void insert(PCB *process) {
	PCB *curr = sleepQueue, *prev = NULL;
	if (!curr) {
		sleepQueue = process;
		process->next = NULL;
		return;
	}
	while (curr) {
		if (curr->timeSlice <= process->timeSlice) {
			process->timeSlice = process->timeSlice - curr->timeSlice;
			if (!curr->next) {
				curr->next = process;
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
			wokenUpProcess->next = NULL;
			sleepQueue = sleepQueue->next;
			ready(wokenUpProcess);
		}
		sleepQueue->timeSlice--;
	}
}
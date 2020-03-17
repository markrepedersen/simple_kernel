#include <xeroskernel.h>
#include <xeroslib.h>

/* sleep.c : sleep device
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
			wokenUpProcess->ret = 0;
			ready(wokenUpProcess);
		}
		sleepQueue->timeSlice--;
	}
}
/* signal.c - support for signal handling
   This file is not used until Assignment 3
 */

#include <xeroskernel.h>
#include <xeroslib.h>

void sigtramp(void (*handler)(void *), void *cntx) {
	kprintf("inside sigtramp\n");
	handler(cntx);
	syssigreturn(cntx);
}

unsigned int pow(int x,int n) {
    unsigned int number = 1;
    for (unsigned int i = 0; i < n; ++i) {
        number *= x;
    }
    return number;
}

void addTrampolineContext(PCB *pcb, int signalNum) {
	signalHandler handler = pcb->signalTable[signalNum];
	if (handler) { // only modify the stack if handler is not ignored
		int *stack_ptr = (int*) pcb->esp;

		// save return value
	    stack_ptr = (int*) (((int) stack_ptr) - sizeof(int));
	    *stack_ptr = pcb->ret;

	    // push current context, 2nd argument of sigtramp, onto stack
	    stack_ptr -= 1;
	    *stack_ptr = (int) pcb->esp;

	    // push handler onto stack
	    stack_ptr -= 1;
	    *stack_ptr = (int) pcb->signalTable[signalNum];

	    // push dummy return address
	    stack_ptr -= 1;
	    *stack_ptr = 0xCAFECAFE;

	    pcb->esp = (void *) ((int) stack_ptr - sizeof(context_frame));

	    context_frame *new_context = pcb->esp;

		memset(new_context, 0xA5, sizeof(context_frame));

	    new_context->ret_eip = (unsigned long) &sigtramp;
	    new_context->iret_cs = getCS();
	    new_context->eflags = 0x00003200;
	    new_context->esp = ((unsigned long) new_context) + 1;
	    new_context->ebp = new_context->esp;

	    pcb->signalMask ^= 1UL << signalNum;
	}
}

/**
* Deliver a process's pending signals in order of priority.
*/
void deliverSignals(PCB *pcb) {
	unsigned int bitmask = pcb->signalMask;
	unsigned int signalNum = 31; // the index of the signal to be delivered
	while (signalNum) {
	    if (bitmask & 1 << 31) { // signal bit is set
	    	addTrampolineContext(pcb, signalNum);
	    	break;
	    } 
	    signalNum--;
	    bitmask = bitmask << 1;
	}
}

void signal(PCB *pcb, int signalNum) {
	if (findProcess(pcb->pid, blockedQueue)) { // signal woke up blocked process
		kprintf("Blocked process: %d was woken up by signal %d\n.", pcb->pid, signalNum);
		pcb->ret = -666;
		removeFromQueue(pcb, &blockedQueue);
		ready(pcb);
	} else if (findProcess(pcb->pid, sleepQueue)) { // signal woke up sleeping process
		kprintf("Sleeping process: %d was woken up by signal %d\n.", pcb->pid, signalNum);
		pcb->ret = pcb->timeSlice * TIME_SLICE;
		removeFromQueue(pcb, &sleepQueue);
		ready(pcb);
	}
	pcb->signalMask = pcb->signalMask | pow(2, signalNum); // schedule signal bitmask for process
	kprintf("Process %d has a pending signal with mask: %d\n", pcb->pid, pcb->signalMask);
}
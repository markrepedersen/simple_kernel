#include <xeroskernel.h>
#include <i386.h>

/* Your code goes here - You will need to write some assembly code. You must
   use the gnu conventions for specifying the instructions. (i.e this is the
   format used in class and on the slides.) You are not allowed to change the
   compiler/assembler options or issue directives to permit usage of Intel's
   assembly language conventions.
*/

void _ISREntryPoint(void);
void _TimerEntryPoint(void);
void _CommonEntryPoint(void);

static void *k_stack;
static unsigned long *ESP;
static int rc, interrupt;
/**
* Add handler to exception table.
*/
void initEvec() {
	(void) k_stack;
	set_evec(INTERRUPT_NUM, (unsigned long) &_ISREntryPoint);
	set_evec(IRQBASE, (unsigned long) &_TimerEntryPoint);
}

int contextswitch(PCB *p) {
	ESP = (unsigned long*) p->esp;
	rc = p->ret;
	kprintf("%d\n", p->esp);

	__asm __volatile (
        "pushf;" /* store flags */
        "pusha;" /* store all registers */
		"movl %%esp, k_stack;" // save kernel stack
		"movl ESP, %%esp;"  // restore process' stack
		"popa;" // restore process' registers
		"movl rc, %%eax;" // return system call result
		"iret;"
		"_TimerEntryPoint:"
		"cli;"
		"pusha;"
		"movl $1, %%ecx;"
		"jmp _CommonEntryPoint;"
		"_ISREntryPoint:"
		"cli;"
		"pusha;"
		"movl $0, %%ecx;"
		"_CommonEntryPoint:"
		"movl %%esp, ESP;"
		"movl k_stack, %%esp;"
		"movl %%eax, rc;"
		"movl %%ecx, interrupt;"
		"popa;"
		"popf;"
		:
		:
		: "eax", "ecx"
		);
	p->esp = (unsigned long) ESP;
	if (interrupt) {
		p->ret = rc;
		context_frame *context = (context_frame*) p->esp;
		context->eax = TIMER_INT;
	}
	return (int) p->esp; // The address of the stack pointer
}

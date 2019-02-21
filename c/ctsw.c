#include <xeroskernel.h>

/* Your code goes here - You will need to write some assembly code. You must
   use the gnu conventions for specifying the instructions. (i.e this is the
   format used in class and on the slides.) You are not allowed to change the
   compiler/assembler options or issue directives to permit usage of Intel's
   assembly language conventions.
*/

void _ISREntryPoint(void);
static void *k_stack;
static unsigned long *ESP;

/**
* Add handler to exception table.
*/
void initEvec() {
	(void) k_stack;
	set_evec(INTERRUPT_NUM, (unsigned long) &_ISREntryPoint);
}

int contextswitch(PCB *p) {
	ESP = (unsigned long*) p->esp;
	__asm __volatile (
        "pushf;" /* store flags */
        "pusha;" /* store all registers */
		"movl %%esp, k_stack;"
		"movl ESP, %%esp;"
		"popa;"
		"iret;"
		"_ISREntryPoint:"
		"pusha;"
		"movl %%esp, ESP;"
		"movl k_stack, %%esp;"
		"popa;"
		"popf;"
		:
		:
		: "eax"
		);
	p->esp = (unsigned long) ESP;
	return (int) p->esp; // The address of the stack pointer
}

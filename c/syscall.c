#include <xeroskernel.h>
#include <stdarg.h>

int syscall(int call, ...) {
	va_list args;
	va_start(args, call);
	int retVal;

	__asm__ __volatile (
		"movl %1, %%eax;"
		"movl %2, %%edx;"
		"int %3;"
		"movl %%eax, %0;"
		: "=g" (retVal)
		: "g" (call), "g" (args), "i" (INTERRUPT_NUM)
		: "eax", "edx");

	return retVal;
}

unsigned int syscreate(functionPointer func, int stack) {
    return syscall(CREATE, func, stack);
}

void sysyield(void) {
	syscall(YIELD);
}

void sysstop(void) {
	syscall(STOP);
}

PID_t sysgetpid(void) {
	return syscall(GET_PID);
}

void sysputs(char* str) {
	syscall(PUT_STRING, str);
}

int syskill(PID_t pid) {
	return syscall(KILL, pid);
}

int syssetprio(int priority) {
	return syscall(PRIORITY, priority);
}

int syssend(PID_t dest_pid, unsigned long num) {
	return syscall(SEND, dest_pid, num);
}

int sysrecv(PID_t *from_pid, unsigned int *num) {
	return syscall(RECEIVE, from_pid, num);
}

unsigned int syssleep( unsigned int milliseconds ) {
	int ticks = milliseconds / TIME_SLICE;
	return syscall(SLEEP, ticks);
}

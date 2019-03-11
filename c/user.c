/* user.c : User processes
 */

#include <xeroskernel.h>

void sendFail(void) {
    kprintf("calling syssend\n");
    syssend(-123, 32);
}

void consume(void) {
    for(int i = 0; i < 15; i++) {
        // kprintf("everyone!\n");
        sysyield();
    }
    sysstop();
}

void produce(void) {
    for(int i = 0; i < 15; i++) {
        // kprintf("Happy 2019 ");
        sysyield();
    }
    sysstop();
}

void spam(void) {
    for (;;) {
        sysputs("hello world\n");
        sysyield();
    }
}

void fallthrough(void) {
    sysputs("This process will fall through.\n");
}

void testSpam(void) {
    int spamPID = syscreate(&spam, 1024);
    kprintf("spam pid is: %d\n", spamPID);
    sysyield();
    sysyield();
    sysyield();
    kprintf("killing spam... should no longer print anything\n");
    syskill(spamPID);
    for(;;) sysyield();
}

void testSetPrio(void) {
    kprintf("priority: %d\n", syssetprio(-1));
    syssetprio(0);
    kprintf("priority: %d\n", syssetprio(-1));
}

void testFallThrough(void) {
    kprintf("Creating fallthrough process.\n");
    syscreate(&fallthrough, 1024);
    sysyield();
    sysyield();
    kprintf("done?\n");
    for(;;) sysyield();
}

void testSendFail(void) {
    sendFail();
}

void root(void) {
    // sysputs("In root\n");
    // sysputs("In root\n");
    // sysputs("In root\n");
    // sysputs("In root\n");
    // sysputs("In root\n");
    // sysputs("In root\n");
    // testSendFail();
    // testFallThrough();
    for (;;) {
        kprintf("root yielding\n");
        sysyield();
    }
}

/* user.c : User processes
 */

#include <xeroskernel.h>

void sendFail(void) {
    kprintf("testing syssend\n");
    kprintf("syssend returned %d, should return -2 on invalid process\n", syssend(990921, 325));
    kprintf("syssend returned %d, should return -3 on send to self\n", syssend(sysgetpid(), 312));
}

void recvAllFail(void) {
    PID_t pid = 0;
    unsigned int num = 123;
    kprintf("recv returned %d, should return -10 when it's the only process\n", sysrecv(&pid, &num));
}

void recvFail(void) {
    kprintf("testing recv\n");
    PID_t pid = 913221;
    unsigned int num = 123;
    kprintf("recv returned %d, should return -2 on invalid process\n", sysrecv(&pid, &num));
    pid = sysgetpid();
    kprintf("recv returned %d, should return -3 on recv from self\n", sysrecv(&pid, &num));
}

void simpleReceiver(void) {
    unsigned int blah = 32;
    PID_t pid = 2;
    int retVal = sysrecv(&pid, &blah);
    kprintf("sysrecv returned %d and received %d\n", retVal, blah);
}

void simpleSender(void) {
    syssend(2, 844);
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
    for(;;);
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

void testReceiveFail(void) {
    recvFail();
}

void simpleSend(void) {
    PID_t pid = syscreate(simpleReceiver, 1024);
    kprintf("created pid %d\n", pid);
    syssend(pid, 90);
}

void simpleRecv(void) {
    PID_t pid = syscreate(simpleSender, 1024);
    unsigned int blah = 0;
    kprintf("created pid %d\n", pid);
    int retVal = sysrecv(&pid, &blah);
    kprintf("sysrecv returned %d and received %d", retVal, blah);
}

void test3(void) {
    PID_t pid = syscreate(fallthrough, 1024);
    int retVal = syssend(pid, 321);
    kprintf("syssend returned %d\n", retVal);
}

void test4_2(void) {
    PID_t pid = syscreate(simpleSender, 1024);
    unsigned int blah = 4141;
    int retVal = sysrecv(&pid, &blah);
    kprintf("sysrecv returned %d\n", retVal);
    retVal = sysrecv(&pid, &blah);
    kprintf("sysrecv returned %d\n", retVal);
}

void test6_1(void) {
    kprintf("sfsdfsd\n");
    kprintf("sfsdfsd\n");
    kprintf("sfsdfsd\n");
    PID_t pid = sysgetpid();
    syskill(pid);
}

void testHardwareInterrupt1(void) {
    while (1) {
        kprintf("This is process 1.\n");
    }
}

void testHardwareInterrupt2(void) {
    while (1) {
        kprintf("This is process 2.\n");
    }
}

void testSleep(void) {
    syssleep(1000);
    kprintf("done sleeping\n");
}

void idleproc( void ) {
    for(;;);
}

void printIsAlive(void) {
    PID_t current_pid = sysgetpid();
    kprintf("Process %d: alive\n", current_pid);
}

void producerConsumerFunc(void) {
    PID_t pid = 2;
    unsigned int message;
    PID_t current_pid = sysgetpid();
    printIsAlive();
    syssleep(5000);
    sysrecv(&pid, &message);
    kprintf("Process %d: received message to sleep for %d ms.\n", current_pid, message);
    syssleep(message);
    kprintf("Process %d done sleeping\n", current_pid);
}

void root(void) {
    PID_t root_pid = sysgetpid();
    PID_t processes[4];
    printIsAlive();
    for (int i = 0; i < 4; ++i) {
        PID_t created_pid = syscreate(&producerConsumerFunc, 1024);
        kprintf("Process %d: created a process with pid: %d\n", root_pid, created_pid);
        processes[i] = created_pid;
    }
    syssleep(4000);
    syssend(processes[2], 10000);
    syssend(processes[1], 7000);
    syssend(processes[0], 20000);
    syssend(processes[3], 27000);
    unsigned int message;
    PID_t proc = processes[3];
    int receiveStatus = sysrecv(&proc, &message);
    kprintf("Process %d: status of receive was %d\n", root_pid, receiveStatus);
    int sendStatus = syssend(processes[2], message);
    kprintf("Process %d: send status was %d\n", root_pid, sendStatus);
    syskill(root_pid);
}



/* user.c : User processes
 */

#include <xeroskernel.h>

void consume(void) {
    for(int i = 0; i < 15; i++) {
        kprintf("everyone!\n");
        sysyield();
    }
    sysstop();
}

void produce(void) {
    for(int i = 0; i < 15; i++) {
        kprintf("Happy 2019 ");
        sysyield();
    }
    sysstop();
}

void root(void) {
    kprintf("Hello world!\n");
    kprintf("creating produce: 0x%x\n", &produce);
    syscreate(&produce, 1024);
    kprintf("creating consume: 0x%x\n", &consume);
    syscreate(&consume, 1024);
    for (;;) {
        // kprintf("root yielding\n");
        sysyield();
    }
}
#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern int entry(void);  /* start of kernel image, use &start    */
extern int end(void);    /* end of kernel image, use &end        */
extern long freemem;    /* start of free memory (set in i386.c) */
extern char *maxaddr;    /* max memory address (set in i386.c)	*/

/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED.  The     ***/
/***   interrupt table has been initialized with a default handler    ***/
/***								      ***/
/***								      ***/
/************************************************************************/

/**
* Initializes the process queues.
*/
void initProcessManager(void) {
    PCB *curr = stoppedQueue = &processTable[0];
    for (int i = 0; i < PROCESS_TABLE_SIZE; i++) {
        curr->pid = i + 1;
        curr->priority = 3;
        if (i == PROCESS_TABLE_SIZE - 1) {
            curr->next = NULL;
            break;
        }
        curr->next = curr + 1;
        curr = (PCB*) curr->next;
    }
}

/*------------------------------------------------------------------------
 *  The init process, this is where it all begins...
 *------------------------------------------------------------------------
 */
void initproc(void) {
    kprintf("\n\nCPSC 415, 2018W2 \n32 Bit Xeros -21.0.0 - even before beta \nLocated at: %x to %x\n", &entry, &end);
    kmeminit();
    initProcessManager();
    initEvec();
    create(&root, 1024);
    dispatch();

    /* This code should never be reached after you are done */
    kprintf("\n\nWhen your kernel is working properly");
    kprintf("this line should never be printed!\n");
    for (;;); /* loop forever */
}
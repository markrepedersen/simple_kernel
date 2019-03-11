/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

/* Symbolic constants used throughout Xinu */

typedef char Bool;        /* Boolean type                  */
typedef unsigned int size_t; /* Something that can hold the value of
                              * theoretical maximum number of bytes
                              * addressable in this architecture.
                              */
#define    FALSE   0       /* Boolean constants             */
#define    TRUE    1
#define    EMPTY   (-1)    /* an illegal gpq                */
#define    NULL    0       /* Null pointer for linked lists */
#define    NULLCH '\0'     /* The null character            */


/* Universal return constants */

#define    OK            1         /* system call ok               */
#define    SYSERR       -1         /* system call failed           */
#define    EOF          -2         /* End-of-file (usu. from read)	*/
#define    TIMEOUT      -3         /* time out  (usu. recvtim)     */
#define    INTRMSG      -4         /* keyboard "intr" key pressed	*/
/*  (usu. defined as ^B)        */
#define    BLOCKERR     -5         /* non-blocking op would block  */

/* Functions defined by startup code */

#define PROCESS_TABLE_SIZE 32

#define PRIORITY_SIZE 4

#define INTERRUPT_NUM 49

typedef int PID_t;

typedef struct {
  unsigned long edi;
  unsigned long esi;
  unsigned long ebp;
  unsigned long esp;
  unsigned long ebx;
  unsigned long edx;
  unsigned long ecx;
  unsigned long eax;
  unsigned long ret_eip;
  unsigned long iret_cs;
  unsigned long eflags;
} context_frame;

typedef void (*functionPointer)(void);

typedef enum {
  CREATE,
  YIELD,
  STOP,
  GET_PID,
  PUT_STRING,
  KILL,
  PRIORITY,
  TIMER_INT
} REQUEST_TYPE;

typedef enum {
  HIGH_PRIORITY,
  HIGH_MEDIUM_PRIORITY,
  LOW_MEDIUM_PRIORITY,
  LOW_PRIORITY
} PRIORITY_LEVEL;

typedef struct PCB {
  PID_t pid;
  int state;
  unsigned long esp;
  unsigned long originalSp;
  struct PCB *next;
  int ret; // return value in case of system call
  int priority;
} PCB;

PCB *readyQueue[PRIORITY_SIZE];
PCB *stoppedQueue;

/**
* A static array containing a list of Process Control Blocks.
* The different process queues will point to different indices of this table.
*/
PCB processTable[PROCESS_TABLE_SIZE];

void bzero(void *base, int cnt);

void bcopy(const void *src, void *dest, unsigned int n);

void disable(void);

unsigned short getCS(void);

unsigned char inb(unsigned int);

void init8259(void);

int kprintf(char *fmt, ...);

void lidt(void);

void outb(unsigned int, unsigned char);

void set_evec(unsigned int xnum, unsigned long handler);

void kmeminit(void);

void *kmalloc(size_t size);

int kfree(void *ptr);

void printCurrentFreeList(void);

void dispatch(void);

int contextswitch(PCB *p);

int create(functionPointer func, int stack );

int syscall( int call, ...  );

unsigned int syscreate(functionPointer func, int stack );

void sysyield( void );

void sysstop( void );

PID_t sysgetpid(void);

void sysputs(char* str);

int syskill(PID_t pid);

int syssetprio(int priority);

void root( void );

void initEvec(void);

/* Anything you add must be between the #define and this comment */
#endif

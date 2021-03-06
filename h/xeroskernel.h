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

#define MAX_SIGNALS 32

#define PRIORITY_SIZE 4

#define INTERRUPT_NUM 49

#define TIME_SLICE 10 // amount of milliseconds

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
  TIMER_INT,
  SEND,
  RECEIVE,
  SLEEP,
  SIGNAL_HANDLER,
  SIGNAL_RETURN,
  SIGNAL_KILL,
  SIGNAL_WAIT
} REQUEST_TYPE;

typedef enum {
  HIGH_PRIORITY,
  HIGH_MEDIUM_PRIORITY,
  LOW_MEDIUM_PRIORITY,
  LOW_PRIORITY
} PRIORITY_LEVEL;

typedef void (*signalHandler)(void *);

typedef enum {
  SIGNAL_IGNORE,
  SIGNAL_CORE,
  SIGNAL_EXIT
} SIGNAL_ACTION;

typedef struct PCB {
  PID_t pid;
  int state;
  unsigned long esp;
  unsigned long originalSp;
  int ret; // return value in case of system call
  int priority;
  int sendValue; // Holds the value this process will send to.
  int timeSlice; // The amount of time slices left for this process if it is sleeping.
  unsigned int* recvLocation; // The location a value should be sent to.
  PID_t* senderPID; // The process that this process is waiting for when receiving.
  char* memoryEnd; // The end of the memory space that the process should be able to access.
  struct PCB *next;
  struct PCB *senders;
  struct PCB *waitingProcess;
  signalHandler signalTable[MAX_SIGNALS];
  unsigned int signalMask; // bitmask to represent the 32 possible pending signals
} PCB;

typedef struct signal_context {
  unsigned long return_address;
  signalHandler handler;
  unsigned long old_esp;
  int return_value; // the return value of the process when the signal went off
  int signalNum;
  context_frame sigreturn_context; // the new context that will be popped into the process's registers once switch from kernel -> process
} signal_context; 

PCB *readyQueue[PRIORITY_SIZE];
PCB *stoppedQueue;
PCB *blockedQueue;
PCB *sleepQueue;

PCB *idleProcess;

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

int syskill(PID_t PID, int signalNumber);

int syssetprio(int priority);

int syssend(PID_t dest_pid, unsigned long num);

int sysrecv(PID_t *from_pid, unsigned int * num);

unsigned int syssleep( unsigned int milliseconds );

int syssighandler(int signal, signalHandler newHandler, signalHandler *oldHandler);

void syssigreturn(void *old_sp);

int syswait(PID_t PID);

void sigtramp(void (*handler)(void *), void *cntx);

void signal(PCB *pcb, int signalNum);

void root( void );

void idleproc( void );

void initEvec(void);

int send(PID_t pid, PCB* process, int value);

int recv(PID_t* pid, PCB* process);

int removeFromQueue(PCB* pcb, PCB** queue);

void addToBack(PCB* pcb, PCB** queue);

void addToFront(PCB *pcb, PCB** queue);

PCB* findProcess(PID_t pid, PCB* queue);

void ready(PCB *pcb);

PCB *findReadyProcess(PID_t pid);

void tick(void);

void sleep(PCB *process);

void addTrampolineContext(PCB *pcb, int signalNum);

void deliverSignals(PCB *pcb);

unsigned int pow(int x,int n);

/* Anything you add must be between the #define and this comment */
#endif

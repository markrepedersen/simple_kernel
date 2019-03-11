/* msg.c : messaging system
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>
/**
 * Completes the message transfer between the two processes, setting the return values of both.
 */
static void sendValue(PCB* sender, PCB* receiver) {
    *(receiver->recvLocation) = sender->sendValue;
    if (*(receiver->senderPID) == 0) *(receiver->senderPID) = sender->pid;
    sender->ret = receiver->ret = 0;
}

// Returns 1 if the process is stopped.
// Assumes pid is not 0.
static int isProcessStopped(PID_t pid) {
    PCB* curr = stoppedQueue;
    while (curr) {
        if ((((pid - 1) % PROCESS_TABLE_SIZE) + 1) == (((curr->pid - 1) % PROCESS_TABLE_SIZE) + 1)) {
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

/**
 * Sends a message to the process with the given PID.
 * Returns 1 if the operation is done, or 0 if the process is now blocked.
 */
int send(PID_t pid, PCB* process, int value) {
    kprintf("process %d sending %d to %d\n", process->pid, value, pid);
    if (pid == process->pid) {
        // Sent message to self.
        kprintf("Process #%d tried to send message to itself\n", pid);
        process->ret = -3;
        return 1;
    }

    if (pid <= 0 || pid > PROCESS_TABLE_SIZE || isProcessStopped(pid)) {
        // Received invalid process ID or ID of a stopped process
        kprintf("Process #%d does not exist.\n", pid);
        process->ret = -2;
        return 1;
    }

    process->sendValue = value;
    PCB* targetProcess = findProcess(pid, blockedQueue);
    if (targetProcess && (*(targetProcess->senderPID) == 0 || *(targetProcess->senderPID) == process->pid)) {
        // Receiver is already waiting on the sender. Send message, then restore receiver.
        sendValue(process, targetProcess);
        removeFromQueue(targetProcess, &blockedQueue);
        ready(targetProcess);
        return 1;
    } else {
        PCB* inBlocked = targetProcess;
        PCB* inReady = findReadyProcess(pid);
        PCB* inSleep = findProcess(pid, sleepQueue);
        targetProcess = (PCB*) ((unsigned long) inBlocked | (unsigned long) inReady | (unsigned long) inSleep);
        PCB* curr = sleepQueue;
        // kprintf("sleep: ");
        // while (curr) {
        //     kprintf("%d ", curr->pid);
        //     curr = curr->next;
        // }
        // kprintf("\n");
        // kprintf("targetprocess: 0x%x\n", targetProcess);
        // kprintf("inblock: 0x%x\n", inBlocked);
        // kprintf("inready: 0x%x\n", inReady);
        // kprintf("insleep: 0x%x\n", inSleep);
        if (!((targetProcess == inBlocked) || (targetProcess == inReady) || (targetProcess == inSleep))) {
            kprintf("something went wrong in send().\n");
            process->ret = -100;
            return 1;
        }
        // Receiver is still active, add sender to the target process' list of senders.
        addToBack(process, &(targetProcess->senders));
        process->senders = NULL;
        process->ret = -1;  // Will be overwritten on success.
        return 0;
    }
}

/**
 * Returns the number of running, ready, or blocked processes.
 */
static unsigned int getNumProcesses(void) {
    // Count processes in the stopped queue.
    unsigned int count = 0;
    PCB* curr = stoppedQueue;
    while (curr) {
        count++;
        curr = curr->next;
    }
    return PROCESS_TABLE_SIZE - count;
}

/**
 * Receives a message from the process with the given PID.
 * Returns 1 if the operation is done, or 0 if the process is now blocked.
 */
int recv(PID_t* pid, PCB* process) {
    if (*pid == process->pid) {
        // Sent message to self.
        kprintf("Process #%d tried to send message to itself\n", *pid);
        process->ret = -3;
        return 1;
    }

    if (*pid != 0 && (*pid < 0 || *pid > PROCESS_TABLE_SIZE || isProcessStopped(*pid))) {
        // Received invalid process ID or ID of a stopped process
        kprintf("Process #%d does not exist.\n", *pid);
        process->ret = -2;
        return 1;
    }

    if ((unsigned long) process->recvLocation < process->originalSp || (char*) process->recvLocation > process->memoryEnd - sizeof(int)) {
        // Bad memory location
        kprintf("Invalid memory location 0x%x given for process %d.\n", process->recvLocation, process->pid);
        process->ret = -4;
        return 1;
    }

    if (getNumProcesses() == 1) {
        kprintf("No other processes are running.\n");
        process->ret = -10;
        return 1;
    }

    process->senderPID = pid;
    if (*pid == 0) {
        if ((unsigned long) pid < process->originalSp || (char*) pid > process->memoryEnd - sizeof(int)) {
            // Bad memory location
            kprintf("Invalid pid memory location 0x%x given for process %d.\n", pid, process->pid);
            process->ret = -5;
            return 1;
        }
        // Receiving from any sender. Check sender list.
        if (process->senders) {
            sendValue(process->senders, process);
            removeFromQueue(process->senders, &(process->senders));
            ready(process->senders);
            return 1;
        } else {
            addToFront(process, &blockedQueue);
            process->ret = -1;
            return 0;
        }
    }

    PCB* targetProcess = findProcess(*pid, process->senders);
    if (targetProcess) {
        sendValue(targetProcess, process);
        removeFromQueue(targetProcess, &(process->senders));
        ready(targetProcess);
        return 1;
    } else {
        addToFront(process, &blockedQueue);
        process->ret = -1;
        return 0;
    }

}

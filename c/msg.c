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
    sender->ret = 0;
    receiver->ret = 0;
}

/**
 * Sends a message to the process with the given PID.
 * Returns 1 if the operation is done, or 0 if the process is now blocked.
 */
int send(PID_t pid, PCB* process, int value) {
    if (pid == process->pid) {
        // Sent message to self.
        kprintf("Process #%d tried to send message to itself\n", pid);
        process->ret = -3;
        return 1;
    }

    PCB* targetProcess = findProcess(pid, stoppedQueue);
    if (pid <= 0 || pid > PROCESS_TABLE_SIZE || targetProcess) {
        // Received invalid process ID or ID of a stopped process
        kprintf("Process #%d does not exist.\n", pid);
        process->ret = -2;
        return 1;
    }

    process->sendValue = value;
    targetProcess = findProcess(pid, blockedQueue);
    if (targetProcess && (*(targetProcess->senderPID) == 0 || *(targetProcess->senderPID) == process->pid)) {
        // Receiver is already waiting on the sender. Send message, then restore receiver.
        sendValue(process, targetProcess);
        removeFromQueue(targetProcess, &blockedQueue);
        // addToBack(targetProcess, &readyQueue);
        ready(targetProcess);
        return 1;
    } else {
        // Receiver is still active, add sender to the target process' list of senders.
        addToBack(process, &(targetProcess->senders));
        addToFront(process, &blockedQueue);
        process->ret = -1;  // Will be overwritten on success.
        return 0;
    }
}

/**
 * Receives a message from the process with the given PID.
 * Returns 1 if the operation is done, or 0 if the process is now blocked.
 */
int recv(PID_t* pid, PCB* process) {
    // check if only process
    // todo

    if (*pid == process->pid) {
        // Sent message to self.
        kprintf("Process #%d tried to send message to itself\n", *pid);
        process->ret = -3;
        return 0;
    }

    PCB* targetProcess = findProcess(*pid, stoppedQueue);
    if (*pid < 0 || *pid > PROCESS_TABLE_SIZE || targetProcess) {
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
            // addToBack(process->senders, &readyQueue);
            ready(process->senders);
            removeFromQueue(process->senders, &blockedQueue);
            removeFromQueue(process->senders, &(process->senders));
            return 1;
        } else {
            addToFront(process, &blockedQueue);
            process->ret = -1;
            return 0;
        }
    }

    targetProcess = findProcess(*pid, blockedQueue);
    if (targetProcess && findProcess(*pid, process->senders)) {
        sendValue(targetProcess, process);
        // addToBack(targetProcess, &readyQueue);
        ready(targetProcess);
        removeFromQueue(targetProcess, &blockedQueue);
        removeFromQueue(targetProcess, &(process->senders));
        return 1;
    } else {
        addToFront(process, &blockedQueue);
        process->ret = -1;
        return 0;
    }

}

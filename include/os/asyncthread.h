#pragma once

/**
 * Register the stack a worker thread runs on.
 *
 * One of the PlayStation 2 thread wrappers the async layer brings up. The four
 * routines in this header belong to another agent's subsystem and are declared
 * here so async.cpp can call them.
 *
 * @param nWorker The worker index.
 * @param pStack The stack buffer.
 * @param nStackSize The stack size in bytes.
 * @ghidraAddress 0x004ff278
 */
void RegisterWorkerStack(int nWorker, void *pStack, int nStackSize);

/**
 * Create the semaphore the async worker waits on.
 *
 * @return The semaphore identifier.
 * @ghidraAddress 0x005369d0
 */
int CreateWorkerSemaphore();

/**
 * Set a worker semaphore's priority.
 *
 * @param nSemaphore The semaphore identifier.
 * @param nPriority The priority.
 * @ghidraAddress 0x00536970
 */
void SetWorkerSemaphorePriority(int nSemaphore, int nPriority);

/**
 * Start a worker thread.
 *
 * @param pfnEntry The thread entry point.
 * @return The thread identifier.
 * @ghidraAddress 0x004ff0c0
 */
int StartWorkerThread(void (*pfnEntry)());

/**
 * Body of the asynchronous read worker.
 *
 * @ghidraAddress 0x00460b28
 */
void AsyncWorkerMain();

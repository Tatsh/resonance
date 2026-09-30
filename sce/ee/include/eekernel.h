#ifndef EEKERNEL_H
#define EEKERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Emotion Engine kernel services: threads, semaphores, interrupt and DMA handlers, caches, alarms,
 * and the GS and OSD configuration calls. Most entries are system calls, and the rest are library
 * routines built on them.
 */

/** Interrupt controller causes for AddIntcHandler() and EnableIntc(). */
enum {
    INTC_GS = 0,       /*!< GS interrupt. */
    INTC_SBUS = 1,     /*!< SBUS interrupt from the IOP. */
    INTC_VBLANK_S = 2, /*!< Start of vertical blank. */
    INTC_VBLANK_E = 3, /*!< End of vertical blank. */
    INTC_VIF0 = 4,     /*!< VIF0 interrupt. */
    INTC_VIF1 = 5,     /*!< VIF1 interrupt. */
    INTC_VU0 = 6,      /*!< VU0 interrupt. */
    INTC_VU1 = 7,      /*!< VU1 interrupt. */
    INTC_IPU = 8,      /*!< IPU interrupt. */
    INTC_TIM0 = 9,     /*!< Timer 0 interrupt. */
    INTC_TIM1 = 10,    /*!< Timer 1 interrupt. */
    INTC_TIM2 = 11,    /*!< Timer 2 interrupt. */
    INTC_TIM3 = 12,    /*!< Timer 3 interrupt. */
    INTC_SFIFO = 13,   /*!< SFIFO interrupt. */
    INTC_VU0WD = 14,   /*!< VU0 watchdog interrupt. */
};

/** DMA controller channels for AddDmacHandler() and EnableDmac(). */
enum {
    DMAC_VIF0 = 0,     /*!< VIF0 channel. */
    DMAC_VIF1 = 1,     /*!< VIF1 channel. */
    DMAC_GIF = 2,      /*!< GIF channel. */
    DMAC_FROM_IPU = 3, /*!< IPU output channel. */
    DMAC_TO_IPU = 4,   /*!< IPU input channel. */
    DMAC_SIF0 = 5,     /*!< SIF0 channel, IOP to EE. */
    DMAC_SIF1 = 6,     /*!< SIF1 channel, EE to IOP. */
    DMAC_SIF2 = 7,     /*!< SIF2 channel. */
    DMAC_FROM_SPR = 8, /*!< Scratchpad output channel. */
    DMAC_TO_SPR = 9,   /*!< Scratchpad input channel. */
    DMAC_CIS = 13,     /*!< Channel interrupt status. */
    DMAC_MEIS = 14,    /*!< Memory FIFO empty interrupt status. */
    DMAC_BEIS = 15,    /*!< Bus error interrupt status. */
};

/** Operations of FlushCache(). */
enum {
    WRITEBACK_DCACHE = 0,  /*!< Write the data cache back to memory. */
    INVALIDATE_DCACHE = 1, /*!< Discard the data cache. */
    INVALIDATE_ICACHE = 2, /*!< Discard the instruction cache. */
    INVALIDATE_CACHE = 3,  /*!< Discard both caches. */
};

/** Base of the uncached view of main memory. */
#define UNCACHED_SEG_BASE 0x20000000U

/** The uncached address of main memory at @p x. */
#define UNCACHED_SEG(x) ((void *)(((unsigned int)(x)) | UNCACHED_SEG_BASE))

/** Parameters and status of a thread. */
struct ThreadParam {
    int status;               /*!< Thread state bits. */
    void (*entry)(void *arg); /*!< Entry point. */
    void *stack;              /*!< Lowest address of the stack. */
    int stackSize;            /*!< Stack size in bytes. */
    void *gpReg;              /*!< Global pointer the thread runs with. */
    int initPriority;         /*!< Starting priority, where a smaller value runs first. */
    int currentPriority;      /*!< Current priority. */
    unsigned int attr;        /*!< Attribute bits. */
    unsigned int option;      /*!< Caller-defined option word. */
    int waitType;             /*!< What the thread waits on. */
    int waitId;               /*!< Semaphore the thread waits on. */
    int wakeupCount;          /*!< Pending wake-up requests. */
};

/** Parameters and status of a semaphore. */
struct SemaParam {
    int currentCount;    /*!< Current count. */
    int maxCount;        /*!< Largest count. */
    int initCount;       /*!< Starting count. */
    int numWaitThreads;  /*!< Threads waiting. */
    unsigned int attr;   /*!< Attribute bits. */
    unsigned int option; /*!< Caller-defined option word. */
};

/** Exits an interrupt handler. Synchronises memory and enables interrupts again. */
#define ExitHandler() __asm__ volatile("sync.l\n\tei")

/** The global pointer the linker assigns. */
extern char _gp[];

/**
 * Sets the GS display mode.
 *
 * @param interlace Non-zero for interlaced output.
 * @param omode Video mode (NTSC, PAL, or a progressive mode).
 * @param ffmd Non-zero to read a field at a time, zero to read a frame.
 * @ghidraAddress 0x005366e0
 */
void SetGsCrt(short interlace, short omode, short ffmd);

/**
 * Ends the program and returns to the system.
 *
 * @param status Exit status.
 * @ghidraAddress 0x00536700
 */
void Exit(int status);

/**
 * Adds an interrupt controller handler.
 *
 * @param cause Interrupt cause (`INTC_*`).
 * @param handler Handler.
 * @param next Position in the handler list (0 first, -1 last).
 * @return Handler identifier, or a negative value on failure.
 * @ghidraAddress 0x005367c0
 */
int AddIntcHandler(int cause, int (*handler)(int cause), int next);

/**
 * Removes an interrupt controller handler.
 *
 * @param cause Interrupt cause (`INTC_*`).
 * @param hid Handler identifier from AddIntcHandler().
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x005367e0
 */
int RemoveIntcHandler(int cause, int hid);

/**
 * Adds a DMA controller handler.
 *
 * @param channel DMA channel (`DMAC_*`).
 * @param handler Handler.
 * @param next Position in the handler list (0 first, -1 last).
 * @return Handler identifier, or a negative value on failure.
 * @ghidraAddress 0x005367f0
 */
int AddDmacHandler(int channel, int (*handler)(int channel), int next);

/**
 * Removes a DMA controller handler.
 *
 * @param channel DMA channel (`DMAC_*`).
 * @param hid Handler identifier from AddDmacHandler().
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536810
 */
int RemoveDmacHandler(int channel, int hid);

/**
 * Unmasks an interrupt cause. Interrupts are disabled around the system call.
 *
 * @param cause Interrupt cause (`INTC_*`).
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00588f80
 */
int EnableIntc(int cause);

/**
 * Masks an interrupt cause. Interrupts are disabled around the system call.
 *
 * @param cause Interrupt cause (`INTC_*`).
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00588f18
 */
int DisableIntc(int cause);

/**
 * Unmasks a DMA channel interrupt. Interrupts are disabled around the system call.
 *
 * @param channel DMA channel (`DMAC_*`).
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00589050
 */
int EnableDmac(int channel);

/**
 * Masks a DMA channel interrupt. Interrupts are disabled around the system call.
 *
 * @param channel DMA channel (`DMAC_*`).
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00588fe8
 */
int DisableDmac(int channel);

/**
 * Calls a handler after a delay. The handler runs in interrupt context.
 *
 * @param time Delay in horizontal blanks.
 * @param handler Handler, given the alarm identifier, the target time, and @p arg.
 * @param arg Argument for the handler.
 * @return Alarm identifier, or a negative value when every alarm is in use.
 * @ghidraAddress 0x00536860
 */
int SetAlarm(unsigned short time,
             void (*handler)(int id, unsigned short time, void *arg),
             void *arg);

/**
 * Creates a thread in the dormant state.
 *
 * @param param Entry point, stack, global pointer, and priority.
 * @return Thread identifier, or a negative value on failure.
 * @ghidraAddress 0x005368e0
 */
int CreateThread(struct ThreadParam *param);

/**
 * Deletes a dormant thread.
 *
 * @param thid Thread identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x005368f0
 */
int DeleteThread(int thid);

/**
 * Starts a dormant thread.
 *
 * @param thid Thread identifier.
 * @param arg Argument for the entry point.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536900
 */
int StartThread(int thid, void *arg);

/**
 * Ends and deletes the calling thread. Does not return.
 *
 * @ghidraAddress 0x00536920
 */
void ExitDeleteThread(void);

/**
 * Ends another thread and returns it to the dormant state.
 *
 * @param thid Thread identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536930
 */
int TerminateThread(int thid);

/**
 * Changes the priority of a thread.
 *
 * @param thid Thread identifier, or 0 for the calling thread.
 * @param priority New priority.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536970
 */
int ChangeThreadPriority(int thid, int priority);

/**
 * Moves the running thread of a priority to the end of its ready queue.
 *
 * @param priority Priority whose queue rotates.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536990
 */
int RotateThreadReadyQueue(int priority);

/**
 * Returns the identifier of the calling thread.
 *
 * @ghidraAddress 0x005369d0
 */
int GetThreadId(void);

/**
 * Reads the status of a thread.
 *
 * @param thid Thread identifier, or 0 for the calling thread.
 * @param info Receives the status.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x005369e0
 */
int ReferThreadStatus(int thid, struct ThreadParam *info);

/**
 * Puts the calling thread to sleep until a wake-up request arrives.
 *
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536a00
 */
int SleepThread(void);

/**
 * Wakes a sleeping thread, or records the request when the thread is awake.
 *
 * @param thid Thread identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536a10
 */
int WakeupThread(int thid);

/**
 * Wakes a sleeping thread from an interrupt handler. A request for the interrupted thread is
 * passed to a helper thread that InitThread() starts.
 *
 * @param thid Thread identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x005f2160
 */
int iWakeupThread(int thid);

/**
 * Suspends a thread.
 *
 * @param thid Thread identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536a50
 */
int SuspendThread(int thid);

/**
 * Starts the helper thread that performs thread requests made from interrupt handlers, and raises
 * the calling thread to priority 1.
 *
 * @return Helper thread identifier, or -1 on failure or when the helper already runs.
 * @ghidraAddress 0x005f2088
 */
int InitThread(void);

/**
 * Returns the end of the heap of the calling thread.
 *
 * @ghidraAddress 0x00536ac0
 */
void *EndOfHeap(void);

/**
 * Creates a semaphore.
 *
 * @param param Starting and largest counts.
 * @return Semaphore identifier, or a negative value on failure.
 * @ghidraAddress 0x00536ae0
 */
int CreateSema(struct SemaParam *param);

/**
 * Deletes a semaphore.
 *
 * @param semid Semaphore identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536af0
 */
int DeleteSema(int semid);

/**
 * Signals a semaphore.
 *
 * @param semid Semaphore identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536b00
 */
int SignalSema(int semid);

/**
 * Signals a semaphore from an interrupt handler.
 *
 * @param semid Semaphore identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536b10
 */
int iSignalSema(int semid);

/**
 * Waits on a semaphore.
 *
 * @param semid Semaphore identifier.
 * @return Non-negative on success, negative on failure.
 * @ghidraAddress 0x00536b20
 */
int WaitSema(int semid);

/**
 * Takes a semaphore without waiting.
 *
 * @param semid Semaphore identifier.
 * @return Non-negative when the semaphore was taken, negative otherwise.
 * @ghidraAddress 0x00536b30
 */
int PollSema(int semid);

/**
 * Reads the packed OSD configuration word (language, time zone, and screen settings).
 *
 * @param config Receives the configuration word.
 * @ghidraAddress 0x00536b90
 */
void GetOsdConfigParam(unsigned int *config);

/**
 * Reads bytes of the extended OSD configuration.
 *
 * @param buffer Receives the bytes.
 * @param size Byte count.
 * @param offset Offset of the first byte.
 * @ghidraAddress 0x00536df0
 */
void GetOsdConfigParam2(void *buffer, int size, int offset);

/**
 * Writes back or discards the caches.
 *
 * @param operation Cache operation (`WRITEBACK_DCACHE`, `INVALIDATE_ICACHE`, and so on).
 * @ghidraAddress 0x00536d60
 */
void FlushCache(int operation);

/**
 * Writes back the data cache lines of a memory range. Interrupts are disabled around the walk.
 *
 * @param start First address of the range.
 * @param end Last address of the range.
 * @ghidraAddress 0x006207d8
 */
void SyncDCache(void *start, void *end);

/**
 * Returns the GS interrupt mask.
 *
 * @ghidraAddress 0x00536e00
 */
unsigned long long GsGetIMR(void);

/**
 * Sets the GS interrupt mask.
 *
 * @param imr New mask.
 * @return Previous mask.
 * @ghidraAddress 0x00536e20
 */
unsigned long long GsPutIMR(unsigned long long imr);

/**
 * Registers the words the kernel writes at the next vertical blank.
 *
 * @param flag Set to 1 at the vertical blank.
 * @param csr Receives the GS status register at the vertical blank.
 * @ghidraAddress 0x00536e50
 */
void SetVSyncFlag(unsigned int *flag, unsigned long long *csr);

/**
 * Installs the handler of a system call.
 *
 * @param number System call number.
 * @param address Handler address.
 * @ghidraAddress 0x00589138
 */
void SetSyscall(int number, void *address);

/**
 * Copies memory in kernel mode through system call 90. InstallSyscallPatch() points the call at
 * KernelCopyHandler().
 *
 * @param dest Destination.
 * @param src Source.
 * @param size Size in bytes, a multiple of 4.
 * @return 0.
 * @ghidraAddress 0x00589148
 */
int KernelCopy(void *dest, const void *src, int size);

/**
 * The handler of system call 90. Copies whole words.
 *
 * @param dest Destination.
 * @param src Source.
 * @param size Size in bytes. A remainder below 4 is not copied.
 * @return 0.
 * @ghidraAddress 0x00589158
 */
int KernelCopyHandler(unsigned int *dest, const unsigned int *src, unsigned int size);

/**
 * Returns the kernel address of an alarm system call, through system call 91 once
 * InstallSyscallPatch() has run.
 *
 * @param number System call number.
 * @return Handler address, or a null pointer.
 * @ghidraAddress 0x00589190
 */
void *GetEntryAddress(int number);

/**
 * Installs the alarm system calls in the kernel. Does nothing when timer 3 already raises compare
 * interrupts.
 *
 * @ghidraAddress 0x005891a0
 */
void InstallSyscallPatch(void);

/**
 * Initialises the library at start-up. Creates the C library semaphores, installs the alarm
 * system calls, and starts the helper thread.
 *
 * @ghidraAddress 0x004b8fe0
 */
void _InitSys(void);

/**
 * Disables interrupts.
 *
 * @return Non-zero when interrupts were enabled.
 * @ghidraAddress 0x005e4510
 */
int DIntr(void);

/**
 * Enables interrupts.
 *
 * @return Non-zero when interrupts were already enabled.
 * @ghidraAddress 0x005e4558
 */
int EIntr(void);

/**
 * Calls a DECI2 service of the kernel.
 *
 * @param function Service number.
 * @param args Argument words.
 * @return Service result.
 * @ghidraAddress 0x00536f10
 */
int Deci2Call(int function, unsigned int *args);

/**
 * Writes a byte to the serial port once its transmit FIFO has room.
 *
 * @param c Byte.
 * @return @p c.
 * @ghidraAddress 0x005fa8c0
 */
int PutSioByte(int c);

/**
 * Formats text to the current console output, the serial port unless scePrintf() is running. The
 * format supports `%c`, `%s`, `%d`, `%u`, `%o`, `%x`, `%e`, and `%f`, the `h` and `l` size
 * prefixes, and a zero-padded width of up to 31 digits.
 *
 * @param format Format string.
 * @ghidraAddress 0x005fb1a0
 */
void PrintfToSioRaw(const char *format, ...);

/**
 * Formats text to the DECI2 kernel console. Output is sent a line at a time.
 *
 * @param format Format string, as PrintfToSioRaw() accepts.
 * @ghidraAddress 0x005fb1d8
 */
void scePrintf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

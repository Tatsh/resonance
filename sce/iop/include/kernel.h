#ifndef KERNEL_H
#define KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * IOP kernel services a module imports from the resident kernel libraries (loadcore, intrman,
 * thbase, and timrman). The layouts follow the stabs the shipped EZMIDI.IRX records.
 */

/** Module identity the loader reads through the `Module` symbol. */
typedef struct _moduleinfo {
    const char *name;       /*!< Module name. */
    unsigned short version; /*!< Major version in the high byte and minor in the low byte. */
} ModuleInfo;

/** Kernel result codes the module compares against. */
enum KernelErrorCode {
    KE_OK = 0,                 /*!< Success. */
    KE_ERROR = -1,             /*!< Unspecified failure. */
    KE_TIMER_NOT_INUSE = -156, /*!< The timer is not running. */
};

/** Interrupt numbers of the IOP interrupt controller and its DMA channels. */
enum INUM {
    INUM_DMA_4 = 36, /*!< SPU2 core 0 DMA completion. */
    INUM_DMA_7 = 40, /*!< SPU2 core 1 DMA completion. */
};

/** Thread attribute for a thread written in C. */
#define TH_C 0x02000000

/** Creation parameters of a thread. */
struct ThreadParam {
    unsigned int attr;   /*!< Attribute bits such as #TH_C. */
    unsigned int option; /*!< Caller-defined option word. */
    void (*entry)(void); /*!< Entry point. */
    int stackSize;       /*!< Stack size in bytes. */
    int initPriority;    /*!< Starting priority, where a smaller value runs first. */
};

/** A 64-bit system clock value in bus cycles. */
typedef struct {
    unsigned int low; /*!< Low word. */
    unsigned int hi;  /*!< High word. */
} SysClock;

/** Timer clock sources for AllocHardTimer() and SetupHardTimer(). */
#define TC_SYSCLOCK 1

/** Counter width a caller requests from AllocHardTimer(), in bits. */
#define TIMER_SIZE_32 32

/** Timer mode bits for SetupHardTimer(). */
#define TM_NO_GATE 0

/** Prescale divisor of one for AllocHardTimer() and SetupHardTimer(). */
#define TIMER_PRESCALE_1 1

/**
 * Enable interrupts on the calling CPU.
 *
 * @return #KE_OK.
 */
int CpuEnableIntr(void);

/**
 * Unmask one interrupt source.
 *
 * @param intrcode Interrupt number, one of #INUM.
 * @return #KE_OK, or a negative error code.
 */
int EnableIntr(int intrcode);

/**
 * Create a dormant thread.
 *
 * @param param Creation parameters.
 * @return The thread identifier, or a negative error code.
 */
int CreateThread(struct ThreadParam *param);

/**
 * Start a dormant thread.
 *
 * @param thid Thread identifier.
 * @param arg Argument the entry point receives.
 * @return #KE_OK, or a negative error code.
 */
int StartThread(int thid, unsigned long arg);

/**
 * Identify the calling thread.
 *
 * @return The thread identifier.
 */
int GetThreadId(void);

/**
 * Sleep until another thread or a handler wakes the calling thread.
 *
 * @return #KE_OK, or a negative error code.
 */
int SleepThread(void);

/**
 * Wake a sleeping thread from interrupt context.
 *
 * @param thid Thread identifier.
 * @return #KE_OK, or a negative error code.
 */
int iWakeupThread(int thid);

/**
 * Read the system clock.
 *
 * @param clock Receives the clock value.
 */
void GetSystemTime(SysClock *clock);

/**
 * Convert microseconds to system clock cycles.
 *
 * @param usec Microseconds.
 * @param clock Receives the cycle count.
 */
void USec2SysClock(unsigned int usec, SysClock *clock);

/**
 * Allocate a hardware timer.
 *
 * @param source Clock source such as #TC_SYSCLOCK.
 * @param size Counter width in bits.
 * @param prescale Prescale divisor.
 * @return The timer identifier, or a negative error code.
 */
int AllocHardTimer(int source, int size, int prescale);

/**
 * Release a hardware timer.
 *
 * @param timid Timer identifier.
 * @return #KE_OK, or a negative error code.
 */
int FreeHardTimer(int timid);

/**
 * Install the compare handler of a hardware timer.
 *
 * @param timid Timer identifier.
 * @param compare Compare value in timer cycles.
 * @param handler Handler run in interrupt context. It returns the next compare value, or zero
 * to stop.
 * @param common Argument the handler receives.
 * @return #KE_OK, or a negative error code.
 */
int SetTimerHandler(int timid,
                    unsigned long compare,
                    unsigned int (*handler)(void *common),
                    void *common);

/**
 * Configure a hardware timer.
 *
 * @param timid Timer identifier.
 * @param source Clock source such as #TC_SYSCLOCK.
 * @param mode Mode bits such as #TM_NO_GATE.
 * @param prescale Prescale divisor.
 * @return #KE_OK, or a negative error code.
 */
int SetupHardTimer(int timid, int source, int mode, int prescale);

/**
 * Start a configured hardware timer.
 *
 * @param timid Timer identifier.
 * @return #KE_OK, or a negative error code.
 */
int StartHardTimer(int timid);

/**
 * Stop a running hardware timer.
 *
 * @param timid Timer identifier.
 * @return #KE_OK, or a negative error code.
 */
int StopHardTimer(int timid);

#ifdef __cplusplus
}
#endif

#endif

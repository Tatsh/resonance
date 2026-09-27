#ifndef OS_SPINLOCK_H
#define OS_SPINLOCK_H

#include <stdbool.h>

/**
 * Interrupt spinlock primitives.
 *
 * The decoded frame queue disables interrupts across its updates through these routines.
 * Their bodies reconstruct with the remaining interrupt work.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Disable interrupts in a spin loop.
 *
 * @return The primitive result, which the queue update ignores. Inferred.
 * @ghidraAddress 0x005e4510
 */
bool SpinDisableInterrupts(void);

/**
 * Re-enable interrupts.
 *
 * @return The primitive result, which the queue update ignores. Inferred.
 * @ghidraAddress 0x005e4558
 */
bool ReenableInterrupts(void);

#ifdef __cplusplus
}
#endif

#endif

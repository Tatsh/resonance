#pragma once

/**
 * Read the free-running cycle counter.
 *
 * Every caller in the image inlines the read, so the image has no out-of-line copy and there is no
 * address to record. On the shipped target the read is the EE `Count` coprocessor register, which
 * is the one genuinely machine-specific part of the timing instrumentation, so a port supplies its
 * own implementation file for this declaration.
 *
 * @return The counter, which wraps.
 */
unsigned ReadCycleCount();

/**
 * Report the milliseconds elapsed since the machine started.
 *
 * The routine accumulates the difference between successive cycle-counter reads into a 64-bit total
 * and divides that total by 294912, the EE clock in cycles per millisecond. Accumulating the
 * differences rather than the reads is what keeps the result correct across a counter wrap.
 *
 * The result is returned as an `int` and widened by its callers, which puts the ceiling at about 24
 * days of running time.
 *
 * The name is inferred from the arithmetic. Nothing in the image attests it, and the routine sits
 * outside this subsystem with a placeholder title still on it in the Ghidra program.
 *
 * @return The elapsed milliseconds.
 * @ghidraAddress 0x00466300
 */
int GetElapsedMilliseconds();

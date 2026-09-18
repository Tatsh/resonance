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

#ifndef EETYPES_H
#define EETYPES_H

/** A 128-bit quadword, the width of the Emotion Engine's registers and of its DMA units. */
typedef unsigned int u_long128 __attribute__((mode(TI)));

/**
 * Store a quadword to a hardware register with one 128-bit sq.
 *
 * The compiler stores a quadword in two 64-bit registers and stores it as two doublewords, and a
 * FIFO register takes the second doubleword at the next address rather than as the upper half. The
 * halves are joined into one register first.
 *
 * @param pDest The register, which must be 16-byte aligned.
 * @param value The quadword to store.
 */
static inline void ee_store_quadword(volatile u_long128 *pDest, u_long128 value) {
    const unsigned long long low = (unsigned long long)value;
    const unsigned long long high = (unsigned long long)(value >> 64);
    unsigned long long joined;

    __asm__ volatile("pcpyld %0, %1, %2\n\tsq %0, 0(%3)"
                     : "=&r"(joined)
                     : "r"(high), "r"(low), "r"(pDest)
                     : "memory");
}

/**
 * Load a quadword from a hardware register with one 128-bit lq.
 *
 * @param pSource The register, which must be 16-byte aligned.
 * @return The quadword read.
 */
static inline u_long128 ee_load_quadword(volatile u_long128 *pSource) {
    unsigned long long low;
    unsigned long long high;

    __asm__ volatile("lq %0, 0(%2)\n\tpcpyud %1, %0, %0"
                     : "=&r"(low), "=&r"(high)
                     : "r"(pSource)
                     : "memory");
    return ((u_long128)high << 64) | low;
}

#endif

#ifndef EEREGS_H
#define EEREGS_H

/**
 * Emotion Engine and GS hardware registers. Each macro is a pointer to its register, as in
 * `*D_STAT = value`.
 */

/** Timer 3 mode register. */
#define T3_MODE ((volatile unsigned int *)0x10001810)

/** DMA channel 0 (VIF0) control register. */
#define D0_CHCR ((volatile unsigned int *)0x10008000)
/** DMA channel 1 (VIF1) control register. */
#define D1_CHCR ((volatile unsigned int *)0x10009000)
/** DMA controller control register. */
#define D_CTRL ((volatile unsigned int *)0x1000e000)
/** DMA controller interrupt status register. */
#define D_STAT ((volatile unsigned int *)0x1000e010)

/** Serial port interrupt status register. */
#define SIO_ISR ((volatile unsigned int *)0x1000f130)
/** Serial port transmit FIFO. */
#define SIO_TXFIFO ((volatile unsigned char *)0x1000f180)

/** GS privileged register that merges the two read circuits. */
#define GS_PMODE ((volatile unsigned long long *)0x12000000)
/** GS privileged register of the video synchronisation mode. */
#define GS_SMODE2 ((volatile unsigned long long *)0x12000020)
/** GS privileged register of the frame buffer read by circuit 1. */
#define GS_DISPFB1 ((volatile unsigned long long *)0x12000070)
/** GS privileged register of the display area of circuit 1. */
#define GS_DISPLAY1 ((volatile unsigned long long *)0x12000080)
/** GS privileged register of the frame buffer read by circuit 2. */
#define GS_DISPFB2 ((volatile unsigned long long *)0x12000090)
/** GS privileged register of the display area of circuit 2. */
#define GS_DISPLAY2 ((volatile unsigned long long *)0x120000a0)
/** GS privileged register of the background colour. */
#define GS_BGCOLOR ((volatile unsigned long long *)0x120000e0)
/** GS system status register. */
#define GS_CSR ((volatile unsigned long long *)0x12001000)

#endif

#pragma once

#include <ezmpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Clear the filled count and the write position.
 *
 * The video decoder worker calls this before it enters its decode loop, so the queue starts
 * empty at the first slot.
 *
 * @param pVoBuf The queue.
 * @ghidraAddress 0x005d3fe8
 */
void voBufReset(VoBuf *pVoBuf);

/**
 * Mark the current slot as decoded and advance the queue.
 *
 * The routine stores 2 in the first word of the tag entry at the write position, increments
 * the filled count, and moves the write position forward modulo the capacity. Interrupts are
 * disabled across the update, and the vertical blank handler releases slots at display time.
 *
 * @param pVoBuf The queue.
 * @ghidraAddress 0x005d4010
 */
void voBufIncCount(VoBuf *pVoBuf);

/**
 * Return the frame store slot at the write position.
 *
 * The decoder stages the next picture here. A null pointer reports that the filled count has
 * reached the capacity, and the caller waits before trying again.
 *
 * @param pVoBuf The queue.
 * @return The write slot, or a null pointer when the queue is full.
 * @ghidraAddress 0x005d4088
 */
void *voBufGetData(VoBuf *pVoBuf);

/**
 * Return the oldest filled tag entry.
 *
 * The display handler reads the returned entry to drive the picture, and the vertical blank
 * handler releases it afterwards. A null pointer reports that no slot is filled.
 *
 * @param pVoBuf The queue.
 * @return The oldest filled entry, or a null pointer when the queue is empty.
 * @ghidraAddress 0x005d40d8
 */
void *voBufGetTag(VoBuf *pVoBuf);

/**
 * Release the oldest filled slot when one is present.
 *
 * The vertical blank handler calls this after the picture has been handed to the display, so
 * the filled count never falls below zero.
 *
 * @param pVoBuf The queue.
 * @ghidraAddress 0x005d4130
 */
void voBufDecCount(VoBuf *pVoBuf);

#ifdef __cplusplus
}
#endif

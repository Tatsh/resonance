#ifndef EZMPEG_VIBUF_H
#define EZMPEG_VIBUF_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Video input buffer of Sony's ezmpeg sample.
 *
 * The buffer holds video bytes in a ring between the file reader and the decoder, and queues one
 * 0x18 byte time stamp per span. The decoder embeds this record at +0x48 of its own state and
 * reaches it through the wrappers in its own unit. Members whose purpose the routines below do
 * not show keep placeholder titles.
 */

/**
 * One queued time stamp, 0x18 bytes.
 *
 * Both stamps are 64 bit wide. The offset counts from the ring base.
 */
typedef struct {
    long long mFirst;  /**< +0x00. First stamp. Inferred. */
    long long mSecond; /**< +0x08. Second stamp. Inferred. */
    int mOffset;       /**< +0x10. Data offset from the ring base. Inferred. */
    int mSize;         /**< +0x14. Data size. Inferred. */
} ViTimeStamp;

/**
 * The video input buffer, 0x60 bytes.
 *
 * Buffered bytes are counted in 2048 byte sectors with a byte remainder, and a semaphore guards
 * every access.
 */
typedef struct {
    unsigned char *mData;           /**< +0x00. Ring base. */
    int mUnknown04;                 /**< +0x04. */
    int mCapacitySectors;           /**< +0x08. Sector budget for free space. Inferred. */
    int mReadSectors;               /**< +0x0c. Read position in sectors. Inferred. */
    int mBufferedSectors;           /**< +0x10. Buffered whole sectors. Inferred. */
    int mBufferedBytes;             /**< +0x14. Buffered bytes past the whole sectors. */
    int mSize;                      /**< +0x18. Ring capacity in bytes. */
    unsigned char mUnknown1c[0x24]; /**< +0x1c. */
    int mSemaId;                    /**< +0x40. Semaphore guarding the record. */
    int mUnknown44;                 /**< +0x44. */
    long long mTotalPut;            /**< +0x48. Bytes accepted through EndPut over the lifetime. */
    ViTimeStamp *mTimeStamps;       /**< +0x50. Stamp ring base. */
    int mTimeStampCapacity;         /**< +0x54. Stamp ring capacity in entries. */
    int mTimeStampCount;            /**< +0x58. Stamps held. */
    int mTimeStampIndex;            /**< +0x5c. Next stamp slot. */
} ViBuf;

/**
 * Lock the buffer and report the writable spans.
 *
 * The write position counts every buffered byte from the consumed sector base, and the free count
 * keeps two sectors in reserve. When the free span reaches past the ring end it is split into the
 * span up to the end and the span restarting at the base, otherwise only the first span is used.
 *
 * @param buffer The buffer to write to.
 * @param firstPut Receives the first writable span.
 * @param firstSize Receives the length of the first span.
 * @param secondPut Receives the second writable span, or null when the span does not wrap.
 * @param secondSize Receives the length of the second span, or zero when the span does not wrap.
 * @ghidraAddress 0x006134e8
 */
void viBufBeginPut(ViBuf *buffer,
                   unsigned char **firstPut,
                   int *firstSize,
                   unsigned char **secondPut,
                   int *secondSize);

/**
 * Unlock the buffer after writing a span.
 *
 * Adds the written size to the buffered byte count and to the lifetime total.
 *
 * @param buffer The buffer written to.
 * @param size The number of bytes written.
 * @ghidraAddress 0x006135e0
 */
void viBufEndPut(ViBuf *buffer, int size);

/**
 * Queue a time stamp for the bytes just written.
 *
 * A helper first discards the stamps the reader has passed. Stamps with both fields negative are
 * then acknowledged without taking a slot, otherwise the stamp is copied into the next slot, the
 * slot index advances around the capacity, and the held count grows. Returns zero only when the
 * stamp ring is already full.
 *
 * @param buffer The buffer owning the stamp ring.
 * @param timeStamp The stamp to queue.
 * @return One when the stamp was queued or carried no time, zero when the ring is full.
 * @ghidraAddress 0x00613638
 */
int viBufPutTs(ViBuf *buffer, ViTimeStamp *timeStamp);

/**
 * Count the buffered bytes.
 *
 * Scales the buffered whole sectors to bytes and adds the buffered byte remainder.
 *
 * @param buffer The buffer to measure.
 * @return The number of bytes currently buffered.
 * @ghidraAddress 0x00613748
 */
int viBufCount(ViBuf *buffer);

/**
 * DMA helpers the input ring uses. Their bodies reconstruct with the DMA library.
 *
 * Each operates on the input record the decoder embeds.
 */

/**
 * Create the queue semaphore for the input record.
 *
 * @param buffer The input record.
 * @param pData Staged data pointer. Inferred.
 * @param pTag Staged tag pointer. Inferred.
 * @param nTagSize Tag size. Inferred.
 * @param pTimeStamps Staged stamp pointer. Inferred.
 * @param nTimeStamps Stamp count. Inferred.
 * @ghidraAddress 0x00613388
 */
void sceDmaCreateQueueSemaphore(
    ViBuf *buffer, void *pData, void *pTag, int nTagSize, void *pTimeStamps, int nTimeStamps);

/**
 * Delete the queue semaphore of the input record.
 *
 * @param buffer The input record.
 * @ghidraAddress 0x00613400
 */
void sceDmaDeleteQueueSemaphore(ViBuf *buffer);

/**
 * Stage the input DMA for the decoder worker.
 *
 * @param buffer The input record.
 * @ghidraAddress 0x006126e8
 */
void sceDmaSub006126e8(ViBuf *buffer);

/**
 * Kick the input DMA after a stall.
 *
 * @param buffer The input record.
 * @ghidraAddress 0x00612890
 */
void sceDmaSub00612890(ViBuf *buffer);

/**
 * Stop the input DMA, saving its position.
 *
 * @param buffer The input record.
 * @ghidraAddress 0x00612b40
 */
void sceDmaSub00612b40(ViBuf *buffer);

/**
 * Restart the input DMA from the saved position.
 *
 * @param buffer The input record.
 * @ghidraAddress 0x00612cc0
 */
void sceDmaSub00612cc0(ViBuf *buffer);

/**
 * Latch the DMA-derived stamp into the callback record.
 *
 * @param buffer The input record.
 * @param pStamps Receives two stamp words. Inferred.
 * @ghidraAddress 0x006131e0
 */
void sceDmaSub006131e0(ViBuf *buffer, long long *pStamps);

/**
 * Queue one stamp that the reader has passed.
 *
 * @param buffer The input record.
 * @param timeStamp The stamp to discard past. Inferred.
 * @ghidraAddress 0x00613088
 */
void sceDmaSub00613088(ViBuf *buffer, ViTimeStamp *timeStamp);

/**
 * Arm the DMA behind the flushed bytes.
 *
 * @param buffer The input record.
 * @ghidraAddress 0x00613798
 */
void sceDmaSub00613798(ViBuf *buffer);

#ifdef __cplusplus
}
#endif

#endif

#include "ezmpeg/vibuf.h"

#include <eekernel.h>
#include <stddef.h>

// Sectors hold 2048 bytes, and BeginPut always keeps two sectors free.
enum {
    kSectorShift = 11,
    kSectorReserve = 2,
};

// 0x006134e8
void viBufBeginPut(ViBuf *buffer,
                   unsigned char **firstPut,
                   int *firstSize,
                   unsigned char **secondPut,
                   int *secondSize) {
    WaitSema(buffer->mSemaId);

    const int writeSectors = buffer->mReadSectors + buffer->mBufferedSectors;
    const int writePosition = (writeSectors << kSectorShift) + buffer->mBufferedBytes;
    const int freeSectors = buffer->mCapacitySectors - buffer->mBufferedSectors - kSectorReserve;
    const int freeSize = (freeSectors << kSectorShift) - buffer->mBufferedBytes;
    const int offset = writePosition % buffer->mSize;
    const int firstLength = buffer->mSize - offset;

    *firstPut = buffer->mData + offset;
    if (firstLength < freeSize) {
        *firstSize = firstLength;
        *secondPut = buffer->mData;
        *secondSize = freeSize - firstLength;
    } else {
        *firstSize = freeSize;
        *secondPut = NULL;
        *secondSize = 0;
    }

    SignalSema(buffer->mSemaId);
}

// 0x006135e0
void viBufEndPut(ViBuf *buffer, int size) {
    WaitSema(buffer->mSemaId);

    buffer->mBufferedBytes += size;
    buffer->mTotalPut += size;

    SignalSema(buffer->mSemaId);
}

// 0x00613638
int viBufPutTs(ViBuf *buffer, ViTimeStamp *timeStamp) {
    int result = 0;

    WaitSema(buffer->mSemaId);
    if (buffer->mTimeStampCount < buffer->mTimeStampCapacity) {
        sceDmaSub00613088(buffer, timeStamp);
        if (timeStamp->mFirst < 0 && timeStamp->mSecond < 0) {
            // Stamps with no time take no slot but still count as handled.
            result = 1;
        } else {
            ViTimeStamp *entry = &buffer->mTimeStamps[buffer->mTimeStampIndex];
            entry->mFirst = timeStamp->mFirst;
            entry->mSecond = timeStamp->mSecond;
            entry->mOffset = timeStamp->mOffset;
            entry->mSize = timeStamp->mSize;
            buffer->mTimeStampCount += 1;
            buffer->mTimeStampIndex = (buffer->mTimeStampIndex + 1) % buffer->mTimeStampCapacity;
            result = 1;
        }
    }
    SignalSema(buffer->mSemaId);

    return result;
}

// 0x00613748
int viBufCount(ViBuf *buffer) {
    WaitSema(buffer->mSemaId);
    const int count = (buffer->mBufferedSectors << kSectorShift) + buffer->mBufferedBytes;
    SignalSema(buffer->mSemaId);

    return count;
}

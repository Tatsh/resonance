#pragma once

#include "memcard/memcardop.h"

/**
 * Move of the read and write position of an open memory-card file.
 *
 * `6SeekOp` in the RTTI descriptor at `0x008ef330`, single inheritance from `MemcardOp` at offset
 * 0. An instance is 0x2c bytes and the vtable is at `0x0082bd00`.
 *
 * The constructor never writes MemcardOp::mPortSlot, which makes this class and CloseOp the two
 * operations that address a descriptor alone. `Memcard::Seek()` has no caller in the image, so
 * nothing exercises the class.
 */
class SeekOp : public MemcardOp {
public:
    /**
     * Construct a seek against an open descriptor.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nFile The descriptor to move.
     * @param nOffset The offset to move by.
     * @param nOrigin The libmc origin, which uses the same three values as `lseek()`.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055eaa8
     */
    SeekOp(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, int nCookie);

    /** @ghidraAddress 0x0055dba8 */
    virtual ~SeekOp();

    /** @ghidraAddress 0x0055ead8 */
    virtual void Issue();

    /** @ghidraAddress 0x0055dbd8 */
    virtual void Complete();

    /**
     * Record the resulting position, or map the failure.
     *
     * On success the body writes mPosition and returns without writing MemcardOp::mStatus, which no
     * constructor initialises. A successful seek therefore reports whatever that word stored when
     * the block was allocated. The behaviour matches the binary.
     *
     * @ghidraAddress 0x0055eb10
     */
    virtual void InterpretResult();

    /** The resulting position, valid once the seek has succeeded. +0x1c */
    int mPosition;

    /** The descriptor to move. +0x20 */
    int mFile;

    /** The offset to move by. +0x24 */
    int mOffset;

    /** The origin the offset is measured from. +0x28 */
    int mOrigin;
};

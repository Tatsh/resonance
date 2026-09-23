#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Open of one memory-card file for reading.
 *
 * `10OpenReadOp` in the RTTI descriptor at `0x008efb40`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x28 bytes and the vtable is at `0x0082bca0`.
 *
 * Issue() passes `sceMcFileAttrReadable` as the open mode, which is the one difference from
 * OpenWriteOp.
 */
class OpenReadOp : public MemcardOp {
public:
    /**
     * Construct an open for reading.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file to open.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055ecc0
     */
    OpenReadOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie);

    /** @ghidraAddress 0x0055ddd0 */
    virtual ~OpenReadOp();

    /** @ghidraAddress 0x0055ed50 */
    virtual void Issue();

    /** @ghidraAddress 0x0055de38 */
    virtual void Complete();

    /**
     * Record the descriptor, or map the failure.
     *
     * The body writes mFile from the result before it tests the sign. A failed open therefore
     * stores the libmc error code in mFile as well as in MemcardOp::mResult. The behaviour matches
     * the binary.
     *
     * @ghidraAddress 0x0055eda0
     */
    virtual void InterpretResult();

    /** The file to open. +0x1c */
    HxStr mPath;

    /** The descriptor, valid once mStatus is kMemcardStatusOk. +0x24 */
    int mFile;
};

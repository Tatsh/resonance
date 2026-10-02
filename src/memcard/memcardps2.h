#pragma once

#include "memcard/memcard.h"

/**
 * PlayStation 2 implementation of the memory-card queue.
 *
 * `10MemcardPS2` in the RTTI descriptor at `0x0090aa70`, single inheritance from `Memcard` at
 * offset 0. An instance is eight bytes, the size of the base alone, and the vtable is at
 * `0x0082bbf0`.
 *
 * The class adds no data. Its whole contribution is Update(), which is where libmc is polled.
 */
class MemcardPS2 : public Memcard {
public:
    /**
     * Construct an empty queue.
     *
     * @ghidraAddress 0x0055e268
     */
    MemcardPS2();

    /** @ghidraAddress 0x0055e300 */
    virtual ~MemcardPS2();

    /**
     * Advance the head of the queue by one step.
     *
     * An unissued head is issued and the call returns, which spends at most one libmc command per
     * step. An issued head is polled with `sceMcSync()` in its non-blocking mode. A finished call
     * has its result recorded on the operation, is reported through MemcardOp::Complete(), and is
     * then destroyed and removed. A head with any other value in MemcardOp::mIssued is ignored.
     *
     * @ghidraAddress 0x0055cfc0
     */
    virtual void Update();
};

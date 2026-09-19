#pragma once

class MemcardCBHandler;

/** Bits MemcardOp::mPortSlot shifts the controller port up by. */
constexpr int kMemcardPortShift = 8;

/** Bits of MemcardOp::mPortSlot that store the slot inside the port. */
constexpr int kMemcardSlotMask = 0xff;

/** MemcardOp::mIssued before the libmc call has been started. */
constexpr int kMemcardOpNotIssued = 0;

/** MemcardOp::mIssued while libmc is servicing the call. */
constexpr int kMemcardOpInFlight = 1;

/**
 * Abstract result a memory-card operation reports through MemcardOp::mStatus.
 *
 * Each operation maps the libmc result code to one of these values in its own
 * InterpretResult() body, and the mapping differs per operation. The same libmc code therefore
 * arrives as different values from different operations, which is why the enumeration is semantic
 * rather than a translation of `sceMcRes*`.
 *
 * Only kMemcardStatusOk, kMemcardStatusNotFormatted, kMemcardStatusCardFull and
 * kMemcardStatusUnknown are established, because those three error values arise from exactly one
 * libmc code across every operation. Every other member's title is inferred from the libmc code
 * that the operations map into it, and no string in the image identifies any of them. The values 8,
 * 9, 12 and 13 are produced by no operation in the image and are therefore absent.
 */
enum MemcardStatus {
    kMemcardStatusOk = 0,           /*!< The operation succeeded. */
    kMemcardStatusNotFormatted = 1, /*!< Always from `sceMcResNoFormat`. */
    kMemcardStatusCardFull = 2,     /*!< Always from `sceMcResFullDevice`. */
    kMemcardStatusNoEntry = 3,      /*!< Inferred. `sceMcResNoEntry` on a path operation. */
    kMemcardStatusBadFile = 4,      /*!< Inferred. `sceMcResNoEntry` on a descriptor operation. */
    kMemcardStatusDenied = 5,       /*!< Inferred. `sceMcResDeniedPermit` on open and delete. */
    kMemcardStatusTooManyOpen = 6,  /*!< Inferred. `sceMcResUpLimitHandle` on open. */
    kMemcardStatusWriteDenied = 7,  /*!< Inferred. `sceMcResDeniedPermit` on write. */
    kMemcardStatusNoDirectory = 10, /*!< Inferred. `sceMcResNoEntry` on a directory listing. */
    kMemcardStatusNoFile = 11,      /*!< Inferred. `sceMcResNoEntry` on delete. */
    kMemcardStatusFailed = 14,      /*!< Inferred. Replace failure on write, non-empty on delete. */
    kMemcardStatusUnknown = 15      /*!< Every libmc code an operation does not map. */
};

/**
 * One queued memory-card operation.
 *
 * `9MemcardOp` in the RTTI descriptor at `0x0086f608`, with no base class, which is why the vptr
 * sits after the data at offset 0x18 rather than at offset 0. An instance is 0x1c bytes and the
 * vtable is at `0x0082c050`.
 *
 * Fourteen classes derive from this one, one per libmc entry point, and each of them supplies the
 * three virtuals below. `Memcard` owns the queue, constructs an operation on the heap for every
 * request, and drives it through Issue(), then Complete(), then destruction.
 *
 * The three method titles are inferred from the bodies. No string in the image identifies any of
 * them. The base bodies at `0x0055f310`, `0x0055f318` and `0x0055f320` are each a single `jr ra`,
 * so all three virtuals are declared with an empty body rather than as pure virtuals.
 *
 * Neither mResult nor mStatus is written by either constructor. An operation that libmc has not
 * finished therefore reports whatever those two words held when the block was allocated.
 */
class MemcardOp {
public:
    /**
     * Construct an operation against one card slot, with no libmc call issued yet.
     *
     * Every derived constructor writes these four members itself rather than delegating, because
     * the compiler inlined this body into all twelve of the operations that address a card. No
     * address of its own survives.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The port shifted up by kMemcardPortShift, with the slot in the low bits.
     * @param pCookie The tag Memcard::Cancel() matches on.
     */
    MemcardOp(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie);

    /**
     * Construct an operation against an open descriptor alone.
     *
     * mPortSlot is not written. `SeekOp` and `CloseOp` are the two operations that use this form,
     * and neither reads mPortSlot.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param pCookie The tag Memcard::Cancel() matches on.
     */
    MemcardOp(MemcardCBHandler *pHandler, void *pCookie);

    /**
     * Release the operation.
     *
     * The body is empty. MemcardPS2::Update() destroys an operation through this slot as soon as
     * Complete() has returned.
     *
     * @ghidraAddress 0x0055f2e0
     */
    virtual ~MemcardOp();

    /**
     * Start the libmc call this operation stands for and record that it is in flight.
     *
     * Every derived body ends by writing 1 to mIssued, which is what moves the operation from the
     * queue head into the polling state.
     *
     * @ghidraAddress 0x0055f310
     */
    virtual void Issue();

    /**
     * Interpret the finished libmc call and report it to the handler.
     *
     * Each derived body calls InterpretResult() on itself and then the one MemcardCBHandler method
     * that matches the operation's type.
     *
     * @ghidraAddress 0x0055f318
     */
    virtual void Complete();

    /**
     * Map mResult to mStatus, and copy any result the call delivered out of it.
     *
     * @ghidraAddress 0x0055f320
     */
    virtual void InterpretResult();

    /**
     * Tag Memcard::Cancel() matches on.
     *
     * Public because Memcard::Cancel() compares this member against its argument directly, and the
     * image exposes no accessor. Every MemcardTask passes its own cookie, so cancelling a task
     * abandons exactly the operations that task queued.
     *
     * +0x04
     */
    void *mCookie;

    /**
     * Zero until Issue() has run, then 1 while libmc is servicing the call.
     *
     * Public because MemcardPS2::Update() reads it directly.
     *
     * +0x08
     */
    int mIssued;

    /**
     * The libmc result, as `sceMcSync()` delivered it.
     *
     * Public because MemcardPS2::Update() writes it directly.
     *
     * +0x0c
     */
    int mResult;

    /**
     * One of MemcardStatus, written by InterpretResult().
     *
     * Public because every MemcardCBHandler implementation reads it directly.
     *
     * +0x10
     */
    int mStatus;

protected:
    // The port shifted up by kMemcardPortShift with the slot in the low bits. Every Issue() body
    // that addresses a card rather than an open descriptor unpacks this member. +0x00
    int mPortSlot;

    // The receiver Complete() reports to. +0x14
    MemcardCBHandler *mHandler;
};

/**
 * Text substituted for an `HxStr` whose buffer is null.
 *
 * Every operation that passes a path to libmc substitutes this pointer when its own `HxStr` is
 * empty, because an empty `HxStr` stores a null buffer. The global sits at `0x006fbd10` and its
 * initial value addresses a lone terminator at `0x008211b8`, which makes it the empty string.
 *
 * Twenty routines outside the memory-card layer read it with the same idiom, so the declaration
 * belongs beside `HxStr` rather than here. It is declared here only because `os/hxstr.h` is
 * another agent's file.
 *
 * @ghidraAddress 0x006fbd10
 */
extern const char *g_pszEmptyString;

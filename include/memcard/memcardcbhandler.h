#pragma once

#include "memcard/checkinfoop.h"
#include "memcard/closeop.h"
#include "memcard/createdirop.h"
#include "memcard/deletefileop.h"
#include "memcard/entspaceop.h"
#include "memcard/formatop.h"
#include "memcard/listdirop.h"
#include "memcard/openreadop.h"
#include "memcard/openwriteop.h"
#include "memcard/readop.h"
#include "memcard/renamefileop.h"
#include "memcard/seekop.h"
#include "memcard/unformatop.h"
#include "memcard/writeop.h"

/**
 * Receiver notified once a queued memory-card operation has finished.
 *
 * `16MemcardCBHandler` in the RTTI descriptor at `0x0086f748`, with no base class and no data
 * members. An instance is four bytes, which is the vptr alone, and the vtable is at `0x007db028`.
 *
 * The interface declares one method per MemcardOp subclass, in the order the operations appear
 * below, and every body is a single `jr ra`. An implementation therefore overrides only the
 * handful of operations it issues. That one-to-one arrangement is what pins each method to its
 * operation: the operation's Complete() body reads the vtable slot directly, and every override in
 * the image reads a member that belongs to the matching operation.
 *
 * Every method title is inferred. No string in the image identifies any of them.
 *
 * `MemcardTask` is the only class in the image that derives from this interface.
 */
class MemcardCBHandler {
public:
    /**
     * Release the receiver.
     *
     * Occupies vtable slot 1. The body is empty.
     *
     * @ghidraAddress 0x00183f30
     */
    virtual ~MemcardCBHandler();

    /**
     * Report a finished card enquiry. Slot 2.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f60
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Report a finished free-entry enquiry. Slot 3.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f68
     */
    virtual void OnEntSpace(EntSpaceOp *pOp);

    /**
     * Report a finished format. Slot 4.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f70
     */
    virtual void OnFormat(FormatOp *pOp);

    /**
     * Report a finished unformat. Slot 5.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f78
     */
    virtual void OnUnformat(UnformatOp *pOp);

    /**
     * Report a finished directory creation. Slot 6.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f80
     */
    virtual void OnCreateDir(CreateDirOp *pOp);

    /**
     * Report a finished directory listing. Slot 7.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f88
     */
    virtual void OnListDir(ListDirOp *pOp);

    /**
     * Report a finished read. Slot 8.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f90
     */
    virtual void OnRead(ReadOp *pOp);

    /**
     * Report a finished write. Slot 9.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183f98
     */
    virtual void OnWrite(WriteOp *pOp);

    /**
     * Report a finished open for writing. Slot 10.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183fa0
     */
    virtual void OnOpenWrite(OpenWriteOp *pOp);

    /**
     * Report a finished open for reading. Slot 11.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183fa8
     */
    virtual void OnOpenRead(OpenReadOp *pOp);

    /**
     * Report a finished close. Slot 12.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183fb0
     */
    virtual void OnClose(CloseOp *pOp);

    /**
     * Report a finished seek. Slot 13.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183fb8
     */
    virtual void OnSeek(SeekOp *pOp);

    /**
     * Report a finished file deletion. Slot 14.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183fc0
     */
    virtual void OnDeleteFile(DeleteFileOp *pOp);

    /**
     * Report a finished rename. Slot 15.
     *
     * @param pOp The finished operation.
     * @ghidraAddress 0x00183fc8
     */
    virtual void OnRenameFile(RenameFileOp *pOp);
};

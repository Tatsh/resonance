#pragma once

#include <list>

class HxStr;
class MemcardCBHandler;
class MemcardOp;

/**
 * Queue of memory-card operations, with one entry point per libmc call.
 *
 * `7Memcard` in the RTTI descriptor at `0x0086f630`, with no base class, which is why the vptr
 * sits after the data at offset 4 rather than at offset 0. An instance is eight bytes and the
 * vtable is at `0x0082c030`, with three slots.
 *
 * Every entry point below constructs one MemcardOp subclass on the heap and appends it to the
 * queue. Nothing is issued at that moment. Update() drives the head of the queue, one libmc call
 * at a time. A caller that queues an open, a write and a close in one turn therefore gets them
 * serviced in order over the following frames. Ordering is what makes the queue rather than a set
 * of direct calls necessary, because libmc services one command at a time.
 *
 * Every entry point takes a `pCookie` tag, which the operation stores and Cancel() matches on.
 * Every MemcardTask passes its own cookie. Abandoning a task therefore abandons exactly the
 * operations that task queued, wherever they sit in the queue.
 *
 * One instance exists. It is a `MemcardPS2` built with `new` inside the constructor at
 * `0x001f2960`, which belongs to a singleton outside this subsystem.
 */
class Memcard {
public:
    /**
     * Construct an empty queue.
     *
     * The body default-constructs the queue and nothing else. The compiler inlined it into
     * `MemcardPS2::MemcardPS2()`. No address of its own survives.
     */
    Memcard();

    /**
     * Release the queue.
     *
     * Occupies vtable slot 1. Any operation still queued is discarded without being issued and
     * without being destroyed, because the body clears the list rather than deleting through it.
     *
     * @ghidraAddress 0x001f6140
     */
    virtual ~Memcard();

    /**
     * Advance the head of the queue by one step.
     *
     * Occupies vtable slot 2. The body of this class is empty, and `MemcardPS2` supplies the
     * PlayStation 2 implementation. The method title is inferred.
     *
     * @ghidraAddress 0x001f61a8
     */
    virtual void Update();

    /**
     * Queue an enquiry about the card in one slot.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047e370
     */
    void CheckInfo(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie);

    /**
     * Queue an enquiry about the free directory entries under one path.
     *
     * No caller exists in the image.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to measure.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047e498
     */
    void EntSpace(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie);

    /**
     * Queue a format of the card in one slot.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047e5d0
     */
    void Format(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie);

    /**
     * Queue an unformat of the card in one slot.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047e6f8
     */
    void Unformat(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie);

    /**
     * Queue the creation of one directory.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to create.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047e820
     */
    void CreateDir(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie);

    /**
     * Queue a listing of one directory.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to list.
     * @param pCookie The tag Cancel() matches on.
     * @param nMode The `sceMcGetDir()` mode.
     * @ghidraAddress 0x0047e990
     */
    void ListDir(MemcardCBHandler *pHandler,
                 int nPortSlot,
                 const HxStr &path,
                 void *pCookie,
                 unsigned nMode);

    /**
     * Queue a read from an open descriptor.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot, which the operation records and never uses.
     * @param nFile The descriptor to read from.
     * @param pBuffer The destination.
     * @param nLength The number of bytes to read.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047ead8
     */
    void Read(MemcardCBHandler *pHandler,
              int nPortSlot,
              int nFile,
              void *pBuffer,
              int nLength,
              void *pCookie);

    /**
     * Queue a write to an open descriptor.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot, which the operation records and never uses.
     * @param nFile The descriptor to write to.
     * @param pBuffer The source.
     * @param nLength The number of bytes to write.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047ec30
     */
    void Write(MemcardCBHandler *pHandler,
               int nPortSlot,
               int nFile,
               const void *pBuffer,
               int nLength,
               void *pCookie);

    /**
     * Queue a move of the position of an open descriptor.
     *
     * No caller exists in the image.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nFile The descriptor to move.
     * @param nOffset The offset to move by.
     * @param nOrigin The origin the offset is measured from.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047ed88
     */
    void Seek(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, void *pCookie);

    /**
     * Queue an open for writing.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file to open.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047eed0
     */
    void OpenWrite(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie);

    /**
     * Queue an open for reading.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file to open.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047f008
     */
    void OpenRead(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie);

    /**
     * Queue a close of an open descriptor.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nFile The descriptor to close.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047f140
     */
    void Close(MemcardCBHandler *pHandler, int nFile, void *pCookie);

    /**
     * Queue the deletion of one file or directory.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file or directory to delete.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047f268
     */
    void DeleteFile(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie);

    /**
     * Queue a rename.
     *
     * No caller exists in the image.
     *
     * @param pHandler The receiver the finished operation reports to.
     * @param nPortSlot The packed port and slot.
     * @param oldPath The existing name.
     * @param newPath The replacement name.
     * @param pCookie The tag Cancel() matches on.
     * @ghidraAddress 0x0047f3a0
     */
    void RenameFile(MemcardCBHandler *pHandler,
                    int nPortSlot,
                    const HxStr &oldPath,
                    const HxStr &newPath,
                    void *pCookie);

    /**
     * Discard every queued operation that was queued with one tag.
     *
     * An operation already in flight is discarded along with the rest, and its libmc call is not
     * waited for.
     *
     * @param pCookie The tag to match.
     * @ghidraAddress 0x0047f4e8
     */
    void Cancel(void *pCookie);

protected:
    // Operations in the order they were queued. Update() services the front of it. +0x00
    std::list<MemcardOp *> mOps;
};

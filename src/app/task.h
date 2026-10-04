#pragma once

#include "app/attachment.h"
#include "os/hxstr.h"

/** The lifecycle position of a Task, stored in its scheduler node. */
enum TaskState {
    kTaskStateIdle = 0,      /*!< Constructed or reset, and not under the scheduler. */
    kTaskStateRunning = 1,   /*!< Linked into the run ring and polled once per pass. */
    kTaskStateSuspended = 2, /*!< Started once and then unlinked, retaining its position. */
    kTaskStateFinished = 3   /*!< Run to completion, and no longer startable. */
};

/**
 * Cooperative unit of work that the run ring polls once per pass.
 *
 * Its RTTI descriptor is at `0x008f07d0`. It derives virtually from Attachment. The virtual
 * base places the Attachment subobject at the end of the most derived object, at `+0x0c` for a
 * standalone Task, and the compiler-generated pointer to it at `+0x00`. Because the only base is
 * virtual, the class also receives a vptr of its own, at `+0x08`, for the five virtuals it
 * declares. The one declared member therefore occupies `+0x04` and the class is 0x14 bytes.
 *
 * The table at `0x00821150` addressed by that vptr runs GetTypeInfo then the five virtuals in
 * declaration order, with no destructor slot. The inherited destructor takes slot 1 of the
 * Attachment table at `0x00821188`, each of whose entries adjusts `this` by `-0x0c`.
 *
 * A task owns one heap-allocated Node, and the run ring links that node rather than the task
 * itself. Start() has two forms. One links the node and returns, after which PollTasks() drives
 * the task; the other runs the task to completion before returning. While a task is linked the
 * ring owns one reference to it, taken in Start() and given back once the task is no longer in
 * kTaskStateRunning.
 *
 * MainLoop is the only class in the image that derives from Task. Every member below other than
 * the constructors and the destructor is unreferenced, because each call site inlined its own
 * copy.
 */
class Task : public virtual Attachment {
public:
    /** One position in the run ring, allocated separately from the task it refers to. */
    struct Node {
        Task *mTask; // +0x00
        int mState;  // +0x04 a TaskState
        Node *mNext; // +0x08
        Node *mPrev; // +0x0c
    };

    /**
     * @ghidraAddress NTSC-U/C: 0x004b6270
     * @ghidraAddress PAL: 0x004f4580
     */
    Task();

    /**
     * Construct a task with its own fresh node.
     *
     * The source task supplies nothing. The body is identical to the default constructor.
     *
     * @param other The task to copy. The binary reads nothing from it.
     * @ghidraAddress NTSC-U/C: 0x004b6708
     * @ghidraAddress PAL: 0x004f4a18
     */
    Task(const Task &other);

    /**
     * Unlink the task and release its node.
     *
     * Destroying a task that is still running reports `hx: destroying active task` and then
     * unlinks it.
     *
     * @ghidraAddress NTSC-U/C: 0x004b6808
     * @ghidraAddress PAL: 0x004f4b18
     */
    virtual ~Task();

    /**
     * Reset the task to kTaskStateIdle.
     *
     * @param other The task to copy, which is compared against this one and otherwise unused.
     * @return This task.
     * @ghidraAddress NTSC-U/C: 0x004b6928
     * @ghidraAddress PAL: 0x004f4c38
     */
    Task &operator=(const Task &other);

    /**
     * Begin running the task.
     *
     * A blocking start polls the task to completion before returning and never links it into the
     * ring. A non-blocking start links the node, takes one reference, and returns.
     *
     * @param bBlocking Non-zero to run the task to completion in this call.
     * @return Zero when the task has already finished or when CheckStateChange() refused.
     * @ghidraAddress NTSC-U/C: 0x004b6370
     * @ghidraAddress PAL: 0x004f4680
     */
    int Start(int bBlocking);

    /**
     * Unlink the task and retain its position.
     *
     * @return Zero when the task is not running and when CheckStateChange() refused.
     * @ghidraAddress NTSC-U/C: 0x004b6968
     * @ghidraAddress PAL: 0x004f4c78
     */
    int Suspend();

    /**
     * Return the task to kTaskStateIdle so that it can be started again.
     *
     * @return Zero when CheckStateChange() refused.
     * @ghidraAddress NTSC-U/C: 0x004b6a28
     * @ghidraAddress PAL: 0x004f4d38
     */
    int Reset();

    /**
     * Retire the task.
     *
     * @return Zero when CheckStateChange() refused.
     * @ghidraAddress NTSC-U/C: 0x004b6ae8
     * @ghidraAddress PAL: 0x004f4df8
     */
    int Finish();

    /**
     * Read the task's lifecycle position.
     *
     * @return A TaskState.
     * @ghidraAddress NTSC-U/C: 0x004b6958
     * @ghidraAddress PAL: 0x004f4c68
     */
    int State() const;

    /**
     * Test whether the task has been started and has not yet finished.
     *
     * @return Non-zero while the state is kTaskStateRunning or kTaskStateSuspended.
     * @ghidraAddress NTSC-U/C: 0x004b65e8
     * @ghidraAddress PAL: 0x004f48f8
     */
    int InProgress() const;

    /**
     * Read how far the task has progressed.
     *
     * @return The value the task reports.
     * @ghidraAddress NTSC-U/C: 0x004b6610
     * @ghidraAddress PAL: 0x004f4920
     */
    float GetProgress();

    /**
     * Read the task's display title.
     *
     * @return The title the task reports.
     * @ghidraAddress NTSC-U/C: 0x004b6638
     * @ghidraAddress PAL: 0x004f4948
     */
    HxStr GetStatus() const;

    /**
     * Test whether the task suppresses its progress report.
     *
     * @return Non-zero to suppress the report.
     * @ghidraAddress NTSC-U/C: 0x004b6670
     * @ghidraAddress PAL: 0x004f4980
     */
    int GetQuiet();

    /**
     * Poll every running task once.
     *
     * One pass advances the ring by one position and polls the task the previous position
     * referred to. A task that reports completion is retired and released. A task that reports
     * more work to do has its progress reported unless it is quiet.
     *
     * @return Non-zero while the ring still includes a task.
     * @ghidraAddress NTSC-U/C: 0x004b6490
     * @ghidraAddress PAL: 0x004f47a0
     */
    static int PollTasks();

protected:
    /**
     * Report how far the task has progressed.
     *
     * The run ring discards the value.
     *
     * @return A fraction of the work completed.
     */
    virtual float Progress() = 0;

    /**
     * Report the task's display title.
     *
     * @return The title.
     */
    virtual HxStr Name() const = 0;

    /**
     * Report whether the task suppresses its progress report.
     *
     * @return Non-zero to suppress the report. The default is zero.
     * @ghidraAddress NTSC-U/C: 0x004b6bb0
     * @ghidraAddress PAL: 0x004f4ec0
     */
    virtual int Quiet();

    /**
     * Approve or refuse a lifecycle change.
     *
     * Every transition consults this before it is applied.
     *
     * @param nFrom The current TaskState.
     * @param nTo The requested TaskState.
     * @return Zero to refuse the change. The default approves every change.
     * @ghidraAddress NTSC-U/C: 0x004b6bb8
     * @ghidraAddress PAL: 0x004f4ec8
     */
    virtual int CheckStateChange(int nFrom, int nTo);

    /**
     * Perform one pass of the task's work.
     *
     * @return Non-zero while work remains.
     */
    virtual int Poll() = 0;

private:
    // Link the node in ahead of the ring head, which makes it the last position of a pass.
    void Link();

    // Take a node out of the ring and make it self-linked again, moving the head along when the
    // head is the node being removed. 0x004b6490 and every
    // transition out of kTaskStateRunning inlined a copy of this.
    static void Unlink(Node *pNode);

    Node *mNode; // +0x04
};

/**
 * Head of the circular run ring, or null when no task is running.
 *
 * @ghidraAddress NTSC-U/C: 0x006fba4c
 * @ghidraAddress PAL: 0x0073f4c4
 */
extern Task::Node *g_pRunningTasks;

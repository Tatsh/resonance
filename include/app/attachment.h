#pragma once

/**
 * Reference-counted root of the engine's polymorphic object graph.
 *
 * `10Attachment` in the RTTI descriptor at `0x0086f5a0`, with no base. The class supplies the
 * virtual destructor that every derived class shares, so a derived class that declares its own
 * virtuals receives a second vptr of its own rather than extending this vtable. The one data word
 * is declared first, which places the vptr after it at `+0x04` and makes the class eight bytes.
 * The destructor is declared ahead of Destroy(), so the table at `0x00821728` runs GetTypeInfo,
 * the destructor, then Destroy().
 *
 * A fresh object starts with one reference. Release() gives one back, and the last release
 * dispatches Destroy(), which deletes the object through the virtual destructor.
 *
 * Callers increment mRefs directly rather than through a method, because the image declares no
 * AddRef. Task::Start() does so on its own object when it hands a task to the run ring.
 * Sch::TimedCommand's constructor at `0x005d32f8` instead increments the count of a separate
 * Sch::Command that it stores, and Sch::Command does not derive from Sch::TimedCommand. Access
 * from an unrelated class is why the field is public rather than protected.
 *
 * Sch::Command, Sch::TempoMap, Sch::TimedCommand, Source, Task, TickTask, TimeTask, MultiMuse, and
 * Phrase all derive from this class. Task derives virtually.
 */
class Attachment {
public:
    Attachment() : mRefs(1) {
    }

    /**
     * @ghidraAddress 0x004bfe30
     */
    virtual ~Attachment();

    /**
     * Give back one reference and destroy the object once the last one is gone.
     *
     * @return The remaining reference count, or zero once the object has been destroyed.
     * @ghidraAddress 0x004bfe60
     */
    int Release();

    /**
     * Destroy the object.
     *
     * The default implementation deletes the object through the virtual destructor.
     *
     * @ghidraAddress 0x004bfea8
     */
    virtual void Destroy();

    int mRefs; // +0x00
};

#pragma once

/**
 * Reference-counted root of the engine's polymorphic object graph.
 *
 * Its RTTI descriptor is at `0x0086f5a0`. It has no base. The class supplies the virtual destructor
 * that every derived class shares. A derived class that declares further virtuals receives a second
 * vptr rather than extending this vtable. The one data word is declared first. The vptr therefore
 * sits after it at `+0x04`, and the class is eight bytes. The destructor is declared ahead of
 * Destroy(). The table at `0x00821728` runs GetTypeInfo, the destructor, then Destroy().
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
     * @ghidraAddress NTSC-U/C: 0x004bfe30
     * @ghidraAddress PAL: 0x004fded0
     */
    virtual ~Attachment();

    /**
     * Give back one reference and destroy the object once the last one is gone.
     *
     * @return The remaining reference count, or zero once the object has been destroyed.
     * @ghidraAddress NTSC-U/C: 0x004bfe60
     * @ghidraAddress PAL: 0x004fdf00
     */
    int Release();

    /**
     * Destroy the object.
     *
     * The default implementation deletes the object through the virtual destructor.
     *
     * @ghidraAddress NTSC-U/C: 0x004bfea8
     * @ghidraAddress PAL: 0x004fdf48
     */
    virtual void Destroy();

    /**
     * Give back one reference to an object that may be null.
     *
     * Passed as a function to std::for_each by the command scheduler, the phrase database, and the
     * playback reader. The body is inline. Its units emit identical copies, at `0x001b8d40` and
     * `0x004ac5e8`. The title is inferred.
     *
     * @param pAttachment The object, or null.
     * @ghidraAddress NTSC-U/C: 0x001b8d40
     * @ghidraAddress PAL: 0x001beb18
     */
    static void ReleaseIfSet(Attachment *pAttachment) {
        if (pAttachment != nullptr) {
            pAttachment->Release();
        }
    }

    int mRefs; // +0x00
};

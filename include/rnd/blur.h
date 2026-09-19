#pragma once

#include <list>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/text.h"
#include "rnd/transformable.h"

namespace Rnd {

/**
 * Motion trail that redraws one drawable at the transforms it recently occupied.
 *
 * `Q23Rnd4Blur` in the RTTI descriptor at `0x008ef068`, whose name string is at `0x00821b28` and
 * whose single base entry at `0x00821b38` records `Rnd::Drawable` at offset 0, non-virtual and
 * public. The class is 0x4c bytes, which the factory at `0x004c3570` proves by allocating exactly
 * that much, and the `Rnd::Object` virtual base subobject sits at `0x30`, which the constructor
 * proves by writing that address into the virtual-base pointer at `+0x00`.
 *
 * Two vtables belong to the class. The four-entry table at `0x00821ae8` is addressed by the
 * `Rnd::Drawable` vptr at `+0x10` and overrides only DrawSelf(), and the eight-entry table at
 * `0x00821aa0` is addressed by the `Rnd::Object` subobject vptr with a `-0x30` adjustment on every
 * entry. Both tables end in an all-zero entry, which is the terminator rather than a null slot.
 *
 * The member titles come from the text DumpText() writes: "[Blur]", "mesh:", " length:",
 * " rate:", "falloff:", and " text:".
 *
 * Ghidra shipped the name `RndBlur__*` on seventeen routines between `0x004b9a68` and `0x004bf858`
 * that belong to `Rnd::String` instead. This class owns only the routines listed below.
 */
class Blur : public Drawable {
public:
    /**
     * One recorded transform.
     *
     * The type is nested because the list stores a copy of the four rows of
     * Rnd::Transformable::mLocalXfm and nothing else in the image stores a transform in a
     * container. Each list node is 0x50 bytes, the two link words followed by eight bytes of
     * padding that place the rows on the 16-byte boundary the VU units read them from.
     */
    struct Xfm {
        float m[kXfmRowCount][kXfmRowFloatCount];
    };

    /**
     * Construct a trail with no subject.
     *
     * The trail length starts at 0, the rate at 1, the falloff at 1.0, and the countdown at the
     * rate. Both subject pointers start null, so the two reference acquisitions the constructor
     * ends with take no effect on a freshly built object.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x004c0e70
     */
    explicit Blur(const HxStr &name);

    /**
     * @ghidraAddress 0x004c0be8
     */
    virtual ~Blur();

    /**
     * Set the mesh the trail is drawn from and discard the recorded transforms.
     *
     * The previous subject loses its reference on this object and the new one gains one. The
     * pointer is stored before the null test, so clearing the mesh stores the null.
     *
     * @param pMesh The mesh to trail, or null for none.
     * @ghidraAddress 0x004c3758
     */
    void SetMesh(Mesh *pMesh);

    /**
     * Set the text the trail is drawn from and discard the recorded transforms.
     *
     * A text subject takes precedence over a mesh subject in DrawSelf().
     *
     * @param pText The text to trail, or null for none.
     * @ghidraAddress 0x004c37b8
     */
    void SetText(Text *pText);

    /**
     * Set how many transforms the trail records and discard the recorded transforms.
     *
     * A negative argument is clamped to 0, which disables the trail.
     *
     * @param nLength The number of transforms to record.
     * @ghidraAddress 0x004c3818
     */
    void SetLength(int nLength);

    /**
     * Set how many frames pass between two recorded transforms and discard the recorded ones.
     *
     * An argument below 1 is clamped to 1.
     *
     * @param nRate The frame interval.
     * @ghidraAddress 0x004c3858
     */
    void SetRate(int nRate);

    /**
     * Set the fraction of the subject's alpha the oldest trail step is drawn with.
     *
     * @param flFalloff The fraction.
     * @ghidraAddress 0x004c34b0
     */
    void SetFalloff(float flFalloff);

    /**
     * Report the mesh the trail is drawn from.
     *
     * @return The mesh, or null when none is set.
     * @ghidraAddress 0x004c3490
     */
    Mesh *GetMesh() const;

    /**
     * Report the text the trail is drawn from.
     *
     * @return The text, or null when none is set.
     * @ghidraAddress 0x004c3498
     */
    Text *GetText() const;

    /**
     * Report how many transforms the trail records.
     *
     * @return The number of transforms.
     * @ghidraAddress 0x004c34a0
     */
    int GetLength() const;

    /**
     * Report how many frames pass between two recorded transforms.
     *
     * @return The frame interval.
     * @ghidraAddress 0x004c34a8
     */
    int GetRate() const;

    /**
     * Report the alpha fraction of the oldest trail step.
     *
     * @return The fraction.
     * @ghidraAddress 0x004c34b8
     */
    float GetFalloff() const;

    /**
     * Write a description of this trail to sink.
     *
     * The two base descriptions come first, and everything below is produced only at a positive
     * dump level. A subject that is not set produces "no object".
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x004bfee0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write this trail's serialised form to stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004c00b0
     */
    virtual void Save(Stream &stream);

    /**
     * Repoint a subject when the object it addressed is replaced.
     *
     * A replacement that is not of the subject's class clears the subject rather than storing a
     * pointer of the wrong type.
     *
     * @param pFrom The object going away.
     * @param pTo The object to store instead, or null.
     * @ghidraAddress 0x004c04c0
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the class key a `.rnd` file writes for a trail.
     *
     * The returned string is g_blurClassName, which the static initialiser at `0x004c32a8` fills
     * with "Blur".
     *
     * @return The class key.
     * @ghidraAddress 0x004c3480
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy the state of pSource into this trail.
     *
     * Both subject pointers are copied without a reference transfer of their own, because the
     * acquisition at the end registers this object against whichever subjects it ends up with.
     *
     * @param pSource The trail to copy from.
     * @param nFlags The set of fields to copy.
     * @ghidraAddress 0x004c35e8
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read this trail's serialised form from stream.
     *
     * A revision of 3 or more is rejected with "Can't load new Blur". The falloff is present from
     * revision 1 and the text subject from revision 2.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004c0258
     */
    virtual void Load(Stream &stream);

    /**
     * Build a trail the class registry vends.
     *
     * @param name The registry key for the new trail.
     * @return The new trail.
     * @ghidraAddress 0x004c3570
     */
    static Blur *NewBlur(const HxStr &name);

    /**
     * Resolve a registry key to a trail.
     *
     * A key that resolves to an object of another class produces null rather than a pointer of
     * the wrong type.
     *
     * @param name The registry key to resolve.
     * @return The trail, or null.
     * @ghidraAddress 0x004c3418
     */
    static Blur *Find(const HxStr &name);

    /**
     * Install the trail factory and register the class key with Rnd::g_manager.
     *
     * @ghidraAddress 0x004c3358
     */
    static void Init();

protected:
    /**
     * Draw the subject once per recorded transform, then once at its own transform.
     *
     * Drawable vtable slot 3. Each step past the first restores one recorded transform onto the
     * subject, sets the subject's material alpha to a value that falls from the material's own
     * alpha towards that alpha scaled by mFalloff, recomposes the world transform, and draws. A
     * recorded transform equal to the current one is skipped. The subject's own transform and the
     * material alpha are restored afterwards. One transform is appended to mXfms every mRate
     * frames, and the list is trimmed to mLength entries.
     *
     * The body is not reconstructed. It writes the material alpha through a virtual of `Rnd::Mat`
     * that `mat.h` does not declare, and it reads the material handle out of reserved runs of
     * `mesh.h` and of `text.h`.
     *
     * @return Non-zero, which draws the children as well.
     * @ghidraAddress 0x004c0638
     */
    virtual int DrawSelf();

private:
    // 0x004c36b0. Registers this object as a referrer of both subjects and discards the recorded
    // transforms. The constructor, Copy(), and Load() are the callers.
    void AcquireObjectRefs();

    // 0x004c3708. Drops this object's registration on both subjects. Copy() and Load() are the
    // callers.
    void ReleaseObjectRefs();

    // Declared in recovered offset order. Every member is private because the image supplies an
    // accessor for each of the five that anything outside the class reads.

    Mesh *mpMesh;         // +0x14
    Text *mpText;         // +0x18
    int mLength;          // +0x1c
    int mRate;            // +0x20
    float mFalloff;       // +0x24
    std::list<Xfm> mXfms; // +0x28
    // Frames still to pass before the next transform is recorded. DrawSelf() counts it down and
    // reloads it from mRate.
    int mCountdown; // +0x2c
};

/**
 * Class key a `.rnd` file writes for a trail.
 *
 * @ghidraAddress 0x006fd248
 */
extern HxStr g_blurClassName;

/**
 * Revision word the reader of the `.rnd` container has most recently consumed.
 *
 * Load() stores the revision here rather than in a local, which is what lets the helpers it calls
 * test it. The word sits one word below Rnd::g_nRndMatLoadVersion in the same pool.
 *
 * @ghidraAddress 0x00894e28
 */
extern int g_nRndBlurLoadRevision;

/**
 * Factory the registered trail creator dispatches through.
 *
 * Init() fills it with NewBlur(), and the thunk the class registry stores loads it rather than
 * calling the factory directly, which is what lets a platform layer substitute a subclass.
 * Rnd::Manager::Init() writes the hook a second time, at `0x00519c98`.
 *
 * A second dispatcher sits at `0x004c3398`. It loads the same hook and returns what the hook
 * produced without the narrowing to Rnd::Object that the registered thunk at `0x004c34e0`
 * performs, so it is the entry point a caller wanting a Rnd::Blur would use. Nothing in the image
 * calls it, and the only reference to it is the exception range table at `0x008693d0`. Its title
 * is not recoverable from the image, so this tree records its address rather than inventing one.
 *
 * @ghidraAddress 0x006fd250
 */
extern Blur *(*g_pfnNewBlur)(const HxStr &name);

} // namespace Rnd

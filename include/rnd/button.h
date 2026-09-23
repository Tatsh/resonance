#pragma once

#include <vector>

#include "os/hxstr.h"
#include "rnd/object.h"

class FailSink;
namespace Rnd {
class Font;
class Mat;
class Mesh;
class Stream;
} // namespace Rnd

namespace Rnd {

class Text;

/**
 * Selectable widget, as one mesh and one text run plus a palette for each.
 *
 * `Q23Rnd6Button` in the RTTI descriptor at `0x008efde0`, with `Rnd::Object` as its only public
 * non-virtual base at offset 0. The class is 0x40 bytes, which the factory at `0x005346f8` pins by
 * requesting exactly that many. The Rnd::Object vptr therefore sits at `+0x18` and this class's
 * members run from `+0x1c` to `+0x3f` with no padding. The eight-entry vtable is at `0x008282d8`,
 * which is the Rnd::Object set exactly; the class declares no virtual of its own.
 *
 * The member titles come from the text DumpText() writes, "state: ", " mesh: ", " text: ",
 * "mats: ", and "fonts: ".
 *
 * What the state selects is not implemented here. SetState() records the index and SetShowing()
 * forwards to both targets, and the class draws nothing and updates nothing, so the two palettes
 * and the index are a record the front end reads. Over two hundred call sites across the metagame
 * and the front-end screens narrow an object to this class, and few of those translation units are
 * reconstructed yet, so which palette entry a given state applies to which of the two targets is
 * recovered nowhere in this tree.
 */
class Button : public Object {
public:
    /**
     * Construct a button with no targets and one empty palette entry each.
     *
     * Both palettes start with a single null entry rather than empty, which the constructor
     * produces by requesting four bytes and then inserting one null.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x00530eb8
     */
    explicit Button(const HxStr &name);

    /**
     * Drop this object's reference on both targets and on every palette entry.
     *
     * @ghidraAddress 0x00530cd0
     */
    virtual ~Button();

    /**
     * Allocate a button under the tag "Rnd::Button".
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x005344b8
     */
    static void *operator new(size_t nSize);

    /**
     * Release a button block under the same tag.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x005344d8
     */
    static void operator delete(void *pBlock);

    /**
     * Write a description of this button to sink.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x005304f0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write this button to stream at revision 0.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00530690
     */
    virtual void Save(Stream &stream);

    /**
     * Retarget both targets and every palette entry that refers to the replaced object.
     *
     * A palette entry whose replacement is null is still overwritten with that null, and the whole
     * of each palette is walked rather than stopping at the first match.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress 0x00530a20
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the class key a `.rnd` file writes for a button.
     *
     * The returned string is the global at `0x0071d8f8`, which the class registration fills with
     * "Button".
     *
     * @return The class key.
     * @ghidraAddress 0x00534620
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy the state and both palettes of pSource into this button.
     *
     * Neither target is copied; both are cleared instead. nFlags is not read, and no base
     * implementation is invoked. A pSource that is not a Button is dereferenced through the null
     * the cast produces rather than rejected.
     *
     * @param pSource The button to copy from.
     * @param nFlags Unread.
     * @ghidraAddress 0x00534780
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read this button from stream.
     *
     * A revision above 0 produces the report "Can't load new Button" and then returns, leaving the
     * object as it was. Unlike the other renderer classes, this one does not transfer control to
     * the abort handler of g_failSink afterwards.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x005307f8
     */
    virtual void Load(Stream &stream);

    /**
     * Show or hide both targets.
     *
     * The argument goes to slot 1 of the Rnd::Drawable table of mMesh and then of mText, and a
     * null target is passed over. The class declares no virtual of its own, so the member is an
     * ordinary method rather than an override.
     *
     * @param nShowing Whether the button draws.
     * @ghidraAddress 0x005349e0
     */
    void SetShowing(int nShowing);

    /**
     * Replace mMesh, moving this object's reference to the new mesh.
     *
     * The new mesh also takes the material mState selects. The routine has no caller in the
     * shipped build.
     *
     * @param pMesh The new mesh, or null.
     * @ghidraAddress 0x00534ad0
     */
    void SetMesh(Mesh *pMesh);

    /**
     * Select one of mMats and one of mFonts.
     *
     * @param nState The state index. MetButtonList passes over an entry whose state is 3, which
     * is how a disabled button is skipped.
     * @ghidraAddress 0x00534a48
     */
    void SetState(int nState);

private:
    // Take a reference on both targets and on every palette entry. Load() and Copy() are its
    // callers. 0x00534820.
    void AddObjectRefs();

    // Drop the reference on both targets and on every palette entry. The destructor, Load(), and
    // Copy() are its callers. 0x00534900.
    void RemoveObjectRefs();

    // Declared in recovered offset order. Every member but mText is private, because nothing in
    // the recovered part of the image touches one of these and the class exposes no accessor.

    int mState;  // +0x1c
    Mesh *mMesh; // +0x20

public:
    /**
     * Label drawn over mMesh.
     *
     * Public rather than private, because MetButtonList::Add() and
     * MetLoadFreqBaseScreen::UpdateNameLabel() both set the label text through a plain load at
     * `+0x24` and the image has no accessor to route either read through. A friend declaration
     * fits the image equally well. +0x24
     */
    Text *mText;

private:
    // Materials the state selects among, for mMesh. The assignment operator this class uses is
    // instantiated out of line at 0x00533260.
    std::vector<Mat *> mMats; // +0x28
    // Fonts the state selects among, for mText. Three of its operations are instantiated out of
    // line, the assignment operator at 0x00533488, the grow-and-fill that insert() and resize()
    // share at 0x00533f88, and the uninitialised fill that grow-and-fill calls at 0x00534fd0.
    // All four addresses are library code, so no body for any of them appears in this tree.
    std::vector<Font *> mFonts; // +0x34
};

/**
 * Allocate and construct a button.
 *
 * The binary bills the allocation to the tag "Rnd::Button" and requests exactly 0x40 bytes.
 *
 * @param name The object name.
 * @return The new button.
 * @ghidraAddress 0x005346f8
 */
Button *NewButton(const HxStr &name);

/**
 * Creator the registered "Button" class builds through.
 *
 * Rnd::Manager::Init() writes NewButton() into the hook directly rather than calling
 * RegisterButtonClass().
 *
 * @ghidraAddress 0x0071d900
 */
extern Button *(*g_pfnNewButton)(const HxStr &name);

/**
 * Build a button through the creator hook.
 *
 * @param name The object name.
 * @return The new button.
 * @ghidraAddress 0x00534538
 */
Button *NewButtonThroughHook(const HxStr &name);

/**
 * Build a button for the registered "Button" class.
 *
 * @param name The object name.
 * @return The new button, as its Rnd::Object subobject.
 * @ghidraAddress 0x00534678
 */
Object *CreateRegisteredButton(const HxStr &name);

/**
 * Point g_pfnNewButton at NewButton() and register the "Button" class with Rnd::Manager.
 *
 * No call site survives in the shipped program, because Rnd::Manager::Init() performs both steps
 * itself. The routine is dead code in the original rather than an unfinished analysis.
 *
 * @ghidraAddress 0x005344f8
 */
void RegisterButtonClass();

/**
 * Registered class name of Rnd::Button, the string "Button".
 *
 * The stub at `0x00534410` is the per-unit static-initialisation glue for this global, invoked
 * with a priority of 0xffff. Called to initialise it constructs the string from the literal
 * "Button" at `0x00828320`, and called to finalise it frees the buffer. The compiler emits the
 * stub, so this tree writes no body for it, only the constructor argument above.
 *
 * @ghidraAddress 0x0071d8f8
 */
extern HxStr g_buttonClassName;

/**
 * Serial revision of the button record currently being read.
 *
 * Load() reads the revision out of the file into this global and then tests it, which is why no
 * store to it appears in the routine. The store happens through the pointer the stream receives.
 *
 * @ghidraAddress 0x0089e060
 */
extern int g_nRndButtonLoadVersion;

} // namespace Rnd

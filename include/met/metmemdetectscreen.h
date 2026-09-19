#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"

/**
 * Base of the screens that probe for a memory card before using one.
 *
 * `19MetMemDetectScreen` in the RTTI descriptor at `0x008eff50`, with two public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00` and MemcardUser at `+140`. The object is 0xa0 bytes, and
 * two children fix that independently, MetMemDetectStartup by placing FadeUser at `+160` and
 * MetMemCardLoadScreen by placing MetMemCardPickerUser at `+160`.
 *
 * Three classes derive from the class, MetLocPickCharScreen, MetMemCardLoadScreen, and
 * MetMemDetectStartup.
 *
 * Two vtables belong to the class, the 44-entry primary at `0x007fccb8` and the 21-entry
 * MemcardUser table at `0x007fcc08` that adjusts `this` by `-140` in every entry. The primary is
 * five entries longer than the MetScreen table, so the class declares five virtuals of its own at
 * slots 39 through 43, at `0x002d8b80`, `0x002db328`, `0x002deaa8`, `0x002deab0`, and
 * `0x002debc0`. The middle two sit eight bytes apart and are two-instruction `jr ra` stubs. None
 * of the five has a recovered name, so all five are recorded rather than declared.
 *
 * The constructor at `0x002deab8` takes the renderer, the load priority, and the three names, and
 * forwards all five to MetScreen. It supplies no literal of its own, which is why all three
 * children pass their own names through it. It writes both vptrs and zeroes the four words below.
 *
 * The destructor at `0x002deb08` restores the primary vptr, restores the MemcardUser vptr to
 * `0x007daf78`, runs the MetScreen destructor, and releases the object with the tag `MsgSink`.
 *
 * Two inherited slots differ from the MetScreen table, 15 at `0x002d9e40` and 26 at `0x002dec10`.
 * The body at `0x002d9e40` is shared by all three children as well, and the body at `0x002db328`
 * fills slot 40 for two of them, so both belong to this class rather than to any child.
 */
class MetMemDetectScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the detector.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name, without the `.rnd` suffix.
     * @ghidraAddress 0x002deab8
     */
    MetMemDetectScreen(MetRenderer *pRenderer,
                       int nPriority,
                       const HxStr &name,
                       const HxStr &directory,
                       const HxStr &file);

    /**
     * @ghidraAddress 0x002deb08
     */
    virtual ~MetMemDetectScreen();

private:
    int mUnknown90; // +0x90
    int mUnknown94; // +0x94
    int mUnknown98; // +0x98
    int mUnknown9c; // +0x9c
};

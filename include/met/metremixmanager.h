#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"
#include "os/asynccallback.h"

class HxStr;

/**
 * Manager of the remix catalogue, which also presents itself as a dialogue screen.
 *
 * `15MetRemixManager` in the RTTI descriptor at `0x008efc50`, with three public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, and AsyncCallback at `+144`. Its
 * own file is `MetRemixManager.cpp`, which its asserts record at `0x00807bb0`, and it is one of
 * only two classes in the subsystem whose file name survives in the image.
 *
 * The object is 0x14c bytes, which the factory at `0x00361020` pins by requesting exactly that
 * many with the tag `MsgSink`. The figure agrees with the recovered member map below, whose last
 * member ends at `+0x14b`. The tag is MsgSink's rather than this class's, because MsgSink is the
 * base that declares `operator new`.
 *
 * The rest of the game resolves the one instance through the accessor at `0x00361000`, which
 * forwards to `0x003610a8`, which runs the lazy initialiser at `0x00361210` and then returns the
 * cached pointer at `0x006c1110`. The initialiser resolves the instance by handing the registry
 * key `MetRemixManager`, at `0x00807a78`, to MetScreen::FindScreenByName(), so the manager is a
 * registered screen rather than a separately constructed singleton. The accessor is not declared
 * below, because the split across three routines does not resolve into one static member without
 * guessing which of the three the programmer wrote.
 *
 * Three vtables belong to the class, the 39-entry primary at `0x00807de0`, the 21-entry
 * MemcardUser table at `0x00807d30` that adjusts `this` by `-140`, and the three-entry
 * AsyncCallback table at `0x00807d10` that adjusts it by `-144`. The primary is the same length as
 * the MetScreen table, so the class declares no virtual of its own.
 *
 * Five entries of the primary table differ from the MetScreen table, which a diff of the two
 * tables settles rather than the title each routine carries. They are 0 `0x00360788`, the
 * compiler-generated GetTypeInfo, 1 `0x00355070` the destructor, 5 `0x00361518`, 15 `0x003555c0`,
 * and 36 `0x00361550`. Slot 36 is a two-instruction bare return at an address the base table does
 * not hold, which is indistinguishable from the base's own empty body re-emitted into this
 * translation unit, so no override is declared for it. An earlier reading counted five while
 * listing three.
 *
 * The MemcardUser table overrides five slots at `0x003569d0`, `0x003573e8`, `0x003553e8`,
 * `0x00355ff0`, and `0x003563e0`, and the AsyncCallback table overrides its one slot at
 * `0x00358310`. All six are declared below with the spelling their base gives them.
 *
 * One of those spellings is now contradicted by the image. The MemcardUser slot 12 override opens
 * with a LogPrintf() of the literal at `0x00807a88`, which reads
 * ` in MetRemixManager::LoadRemixCB(). Return code `. The method is therefore named `LoadRemixCB`
 * and MemcardUser spells slot 12 `OnRemixLoaded`, a title that header records as inferred from the
 * task that reports through the slot rather than from any string. The declaration below retains
 * the base spelling, because an override that differs from its base by one letter is a new virtual,
 * and correcting the base is a change to MemcardUser and to every other class that overrides the
 * slot.
 *
 * This declaration is deliberately partial and declares no data member. The constructor at
 * `0x00352b80` runs for roughly 300 instructions, and everything after the three vptr writes is
 * member initialisation. The recovered map follows. A red-black tree header occupies `+0x94`
 * through `+0x9c`, built by taking a 0x20-byte node from the STL pool and self-linking it, and a
 * second occupies `+0xa0` through `+0xa8` over a 0x18-byte node. A 0x20-byte node is a 0x10-byte
 * tree-node base plus a 0x10-byte value, and a 0x18-byte node is the same base plus an 8-byte
 * value, which is the width of one `HxStr`. An earlier reading placed the first tree at `+0x98`.
 * Vectors follow at `+0xac` and `+0xb8`. A nested object occupies `+0xc4` through `+0xd3`, with a
 * vector at its own `+0x00` and, following the g++ 2.x layout for a class with no base, its vptr
 * at `+0x0c`, set to `0x007e6d90`; the constructor then runs the routine at `0x001e2248` on it.
 * Words at `+0xd4`, `+0xd8`, `+0xe0`, `+0xe4`, `+0xec`, `+0xf0`, and `+0xf4` start at zero,
 * `+0xdc` starts at one, and `+0xe8` starts at -1. Two further vectors follow at `+0xf8` and
 * `+0x104`, neither of whose first word the constructor writes. A 0x38-byte record occupies
 * `+0x114` through `+0x14b`, the same record MetSaveRemix stores a vector of, with four `HxStr`
 * members built from the empty string at `0x008077d8`, a byte at `+0x134` set to one, and words at
 * `+0x138` and `+0x148`. A vector at `+0x13c` completes it.
 *
 * Four of those words are read by the slots declared below. The AsyncCallback slot compares its
 * first argument against `+0xe4`, the two playlist slots write `+0xdc` and compare `+0xd4` against
 * `+0xd8`, and the remix-loaded slot branches on `+0xf4` against 0 and 1. The second tree at
 * `+0xa0` is the one the remixes-listed slot looks a port and slot up in.
 *
 * The constructor body is not written. The element classes of the two trees, of the five vectors,
 * of the nested object at `+0xc4`, and of the 0x38-byte record are all unidentified, so no member
 * can be declared with a type that reproduces its initialisation.
 *
 * The destructor at `0x00355070` destroys the 0x38-byte record at `+0x114`, then the vectors at
 * `+0xf8` and `+0x104`, then the nested object at `+0xc4`, then the vectors at `+0xb8` and
 * `+0xac`, then the two trees, restores the AsyncCallback vptr to `0x007f7e78` and the MemcardUser
 * vptr to `0x007daf78`, runs the MetScreen destructor, and releases the object with the tag
 * `MsgSink`. Every step is compiler-generated member destruction or a vptr restore, so the
 * definition is empty.
 */
class MetRemixManager : public MetScreen, public MemcardUser, public AsyncCallback {
public:
    /**
     * Construct the manager.
     *
     * Supplies `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for
     * the container. The body is not written, for the reason recorded in the class documentation.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00352b80
     */
    MetRemixManager(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00355070
     */
    virtual ~MetRemixManager();

    /**
     * Hide the dialogue view instead of showing it. Slot 5.
     *
     * The whole body is one call. It dispatches Rnd::Drawable::SetShowing() with a zero argument on
     * the Drawable subobject of MetScreen::mUnknown14, at `+0x18` within the view, which is the
     * same subobject MetScreen::Draw() forwards to. The view is dereferenced with no null check,
     * and nothing is shown, which suits a manager that registers as a screen only to receive
     * messages.
     *
     * The body is not written because MetScreen still declares mUnknown14 private. That member is
     * read by the code of a derived class here, so it belongs in the protected section on the same
     * reasoning that already moved mUnknown10 and mUnknown18 there.
     *
     * @ghidraAddress 0x00361518
     */
    virtual void EnterAndShow();

    /**
     * Act on the choice the user made in one of the manager's dialogues. Slot 15.
     *
     * The body is not written. It compares the dialogue name against a chain of literals through
     * HxStr::MatchesLiteral, among them `mem_load_remix_data`, `mem_load`, `mem_format_check`,
     * `mem_format_go`, `mem_format_done`, `save_fail`, and `load_fail`, and branches each match on
     * the choice. The chain runs for roughly 0x630 instructions and every branch drives memcard
     * routines that are not identified.
     *
     * @param name The dialogue the screen requested, which the message screen reports back.
     * @param nChoice Which of the dialogue's buttons the user chose, counted from zero.
     * @ghidraAddress 0x003555c0
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Act on the format the card reported. MemcardUser slot 5.
     *
     * The body is not written. A zero status raises the `mem_format_done` dialogue and a status of
     * 13 takes a second path from `0x00356c18`, and every other status returns. The port and slot
     * argument is not read. The failure dialogues the routine can raise are `format_fail`,
     * `format_success`, and `format_already`.
     *
     * @param nPortSlot Which card port and slot reported, which the body does not read.
     * @param nStatus Zero on success, and 13 for the one failure the second path covers.
     * @ghidraAddress 0x003569d0
     */
    virtual void OnCardFormatted(int nPortSlot, int nStatus);

    /**
     * Act on the playlist save the card reported. MemcardUser slot 10.
     *
     * The body is not written. It branches on the status against 1 and 2 and raises one of
     * `playlist_save_failed` and `playlist_save_failed_tryagain`, and the port and slot argument is
     * not read.
     *
     * @param nPortSlot Which card port and slot reported, which the body does not read.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x003573e8
     */
    virtual void OnJukeboxPlayListSaved(int nPortSlot, int nStatus);

    /**
     * Record the remixes one card slot reported. MemcardUser slot 11.
     *
     * The body is not written. It looks the port and slot up in the red-black tree at `+0xa0`
     * through the routine at `0x003445f0`, which is not identified, and then merges what the card
     * reported into the catalogue.
     *
     * @param nPortSlot Which card port and slot reported.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x003553e8
     */
    virtual void OnRemixesListed(int nPortSlot, int nStatus);

    /**
     * Act on the remix load the card reported. MemcardUser slot 12.
     *
     * The image names this method `LoadRemixCB`, and the class documentation records why the
     * declaration retains the base spelling.
     *
     * The body is not written. It opens by writing the status to the log through the literal at
     * `0x00807a88`, and a non-zero status then raises the `remix_load_failed` dialogue with the
     * message `Failed to load remix from memory card slot %i.`. A zero status branches on the word
     * at `+0xf4` against 0 and 1. The port and slot argument reaches the diagnostic message and
     * nothing else.
     *
     * @param nPortSlot Which card port and slot reported, which the failure message formats.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x00355ff0
     */
    virtual void OnRemixLoaded(int nPortSlot, int nStatus);

    /**
     * Act on the playlist load the card reported. MemcardUser slot 15.
     *
     * The body is not written. It writes one into the word at `+0xdc` on every path. A zero status
     * that also finds the two counters at `+0xd4` and `+0xd8` equal exits `MetMsgScreen`, and a
     * non-zero status takes a second path from `0x00356450`. The port and slot argument is not
     * read.
     *
     * @param nPortSlot Which card port and slot reported, which the body does not read.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x003563e0
     */
    virtual void OnJukeboxPlayListLoaded(int nPortSlot, int nStatus);

    /**
     * Act on the asynchronous read the file layer finished. AsyncCallback slot 2.
     *
     * The body is not written. A handle that differs from the word at `+0xe4` returns at once
     * through `0x00358428`, which is how the manager ignores a completion it did not request. The
     * matching path resolves the game manager and runs for roughly 0x400 instructions parsing the
     * buffer, and it reads the length as well as the handle.
     *
     * @param nHandle The request the completion belongs to, compared against the word at `+0xe4`.
     * @param nFile The file the request read from.
     * @param pBuffer The bytes the request read.
     * @param nLength How many bytes arrived.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x00358310
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);
};

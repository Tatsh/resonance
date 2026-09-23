#pragma once

#include <map>
#include <vector>

#include "game/jukeboxplaylist.h"
#include "memcard/memcarduser.h"
#include "met/metremixrecord.h"
#include "met/metscreen.h"
#include "os/asynccallback.h"
#include "os/hxstr.h"

class MetRenderer;

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
 * The rest of the game resolves the one instance through shared(), which resolves it lazily by
 * handing the registry key `MetRemixManager` to MetScreen::FindScreenByName(). The manager is a
 * registered screen rather than a separately constructed singleton.
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
 * The constructor at `0x00352b80` is member initialisation after the three vptr writes, and the
 * destructor at `0x00355070` is compiler-generated member destruction in reverse order, so the
 * member list below reproduces both. The vector at `+0xf8` is the g++ 2.x `bit_vector`, whose two
 * iterators each carry an empty base word, which is why it spans 0x1c bytes. The first tree's
 * teardown at `0x0035ef90` releases a vector of MetRemixRecord in each node, and the second's at
 * `0x003619a0` releases nothing, which fixes their value types.
 */
class MetRemixManager : public MetScreen, public MemcardUser, public AsyncCallback {
public:
    /**
     * Construct the manager.
     *
     * Supplies `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for
     * the container.
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
     * Build the manager on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new manager.
     * @ghidraAddress 0x00361020
     */
    static MetRemixManager *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Return the one instance, resolving it through the screen registry on first use.
     *
     * @return The registered manager, or null before it is registered.
     * @ghidraAddress 0x00361000
     */
    static MetRemixManager *shared();

    /**
     * Report the current remix record.
     *
     * The stats screens and MetSaveRemixScreen read it. The title is inferred.
     *
     * @return The record at `+0x114`.
     * @ghidraAddress 0x00361558
     */
    MetRemixRecord *GetRecord();

    /**
     * Replace the current remix record.
     *
     * MetRemixLoadScreen::OnUnknownSlot36() is the caller. The title is inferred.
     *
     * @param record The record to copy.
     * @ghidraAddress 0x00361560
     */
    void SetRecord(const MetRemixRecord &record);

    /**
     * Look a remix up by name in the catalogue.
     *
     * A record whose word at `+0x24` is non-zero matches only in the factory set, keyed -1, and a
     * record whose word is zero matches only in a card slot's set. The title is inferred.
     *
     * @param name The name compared against each record's second string.
     * @return The first matching record, or null.
     * @ghidraAddress 0x00359230
     */
    MetRemixRecord *FindRecord(const HxStr &name);

    /**
     * Look a remix up by name for the playlist editor.
     *
     * The body is FindRecord() alone. MetJukeboxEditPlaylistScreen is the caller. The title is
     * inferred.
     *
     * @param name The remix name.
     * @return The matching record, or null.
     * @ghidraAddress 0x003612c0
     */
    MetRemixRecord *LookupRemix(const HxStr &name);

    /**
     * Drop the playlist entries the catalogue no longer knows.
     *
     * MetJukeboxTopButtonsScreen is the caller. The title is inferred.
     *
     * @ghidraAddress 0x003612a0
     */
    void PrunePlayList();

    /**
     * Start loading the current playlist track.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x003612e0
     */
    void PlayCurrentTrack();

    /**
     * Step to the previous playlist track, or to a random one in shuffle mode.
     *
     * The step stops at the first track. The image records no caller. The title is inferred.
     *
     * @ghidraAddress 0x00361300
     */
    void PreviousTrack();

    /**
     * Step to the next playlist track, or to a random one in shuffle mode.
     *
     * The step wraps from the last track to the first. Inline. StartLoadedRemix() expands it, and
     * the out-of-line copy has no caller. The title is inferred.
     *
     * @ghidraAddress 0x00361358
     */
    inline void NextTrack();

    /**
     * Pick a random track that has not played since the last reset.
     *
     * Every track is marked unplayed again once all have played. The title and the member it
     * sets are attested by the log line the routine writes.
     *
     * @ghidraAddress 0x0035a7e0
     */
    void RandomTrack();

    /**
     * Start playing the playlist.
     *
     * Records MetJukeboxTopButtonsScreen and MetHelpScreen as the screens to restore after a
     * track, clears one played flag per track, rewinds, picks a random first track in shuffle
     * mode, records the caller's screens, and loads the current track.
     * MetJukeboxEditPlaylistScreenDone slot 36 is the caller. The title is inferred.
     *
     * @param returnScreens The screens the caller wants restored.
     * @param nShuffle Non-zero to play in random order.
     * @ghidraAddress 0x003593d0
     */
    void StartPlayList(const std::vector<HxStr> &returnScreens, int nShuffle);

    /**
     * Clear the jukebox flag in the game parameters and rewind the playlist.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x0035a6b0
     */
    void LeaveJukeboxMode();

    /**
     * Hide the dialogue view instead of showing it. Slot 5.
     *
     * The whole body is one call. It dispatches Rnd::Drawable::SetShowing() with a zero argument on
     * the Drawable subobject of MetScreen::mUnknown14, at `+0x18` within the view, which is the
     * same subobject MetScreen::Draw() forwards to. The view is dereferenced with no null check,
     * and nothing is shown, which suits a manager that registers as a screen only to receive
     * messages.
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
     * Records the status under the port and slot in mListStatus and counts one more listing in
     * mUnknownd4. Once that count reaches mUnknownd8 while mUnknowndc is set, the `MetMsgScreen`
     * dialogue is exited. The status branches against 0 and 3 lead to the same code.
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
     * Sets mUnknowndc on every path, and exits `MetMsgScreen` when the two counters mUnknownd4 and
     * mUnknownd8 agree. The zero and non-zero status branches lead to the same code. The port and
     * slot argument is not read.
     *
     * @param nPortSlot Which card port and slot reported, which the body does not read.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x003563e0
     */
    virtual void OnJukeboxPlayListLoaded(int nPortSlot, int nStatus);

    /**
     * Act on the asynchronous read the file layer finished. AsyncCallback slot 2.
     *
     * A completed remix read is copied into the reset log and started, or its dialogue exited, as
     * mUnknownf4 selects. A completed index read is parsed into records filed under the slot key
     * mUnknowne8, the buffer is released, and the listing is counted as OnRemixesListed() counts
     * it. Any other handle is ignored.
     *
     * @param nHandle The request the completion belongs to, compared against the word at `+0xe4`.
     * @param nFile The file the request read from.
     * @param pBuffer The bytes the request read.
     * @param nLength How many bytes arrived.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x00358310
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);

private:
    // The key of the factory remixes in mRemixes. Card slots use their own port-and-slot keys.
    static constexpr int kFactorySlot = -1;

    // 0x003610a8
    static MetRemixManager *ResolveSharedInstance();
    // 0x00361210
    static void CacheSharedInstance();
    // 0x00357f80. Starts reading the remix index file, recording the request in mIndexRequest.
    void LoadIndex();
    // 0x003580f0. Starts reading one remix file, recording the request in mRemixRequest.
    void LoadRemixFile(const HxStr &fileName);
    // 0x00361418. A factory remix is read from its file and any other through the memory card.
    // LoadCurrentTrack() expands it inline.
    inline void LoadRemix(const MetRemixRecord &record, int nFactory);
    // 0x00361480
    void LoadCurrentTrack();
    // 0x0035abf8. Starts a jukebox game on the current track's remix in a random arena, burns the
    // recorded appearances, steps the playlist, and brings up MetLoadGameScreen. Both remix-load
    // completions call it. The title is inferred.
    void StartLoadedRemix();
    // Replaces mUnknownac with the caller's screens by clearing, resizing, and then assigning.
    // StartPlayList() and the playlist save at 0x00356508 expand it identically.
    inline void SetReturnScreens(const std::vector<HxStr> &screens);
    // 0x003613d8. Clamps into zero through the track count, which admits one past the end.
    void SetCurrentTrack(int nTrack);
    // 0x003610d0. Pushes every screen named in mUnknownb8 and activates the first.
    void PushUnknownb8Screens();
    // 0x00361170. Pushes every screen named in mUnknownac and activates the first.
    void PushUnknownacScreens();

    // 0x006c1110
    static MetRemixManager *sInstance;

    std::map<int, std::vector<MetRemixRecord>> mRemixes; // +0x94, keyed by card slot
    std::map<int, int> mListStatus;                      // +0xa0, by port and slot
    std::vector<HxStr> mUnknownac;                       // +0xac
    std::vector<HxStr> mUnknownb8;                       // +0xb8
    JukeboxPlayList mPlayList;                           // +0xc4
    int mUnknownd4;                                      // +0xd4
    int mUnknownd8;                                      // +0xd8
    int mUnknowndc;                                      // +0xdc, starts at one
    int mIndexRequest;                                   // +0xe0
    int mRemixRequest;                                   // +0xe4, matched by Done()
    // The mRemixes key Done() files the index under. Starts at -1.
    int mUnknowne8;                  // +0xe8
    int mCurrentPlaylistTrack;       // +0xec
    int mShuffle;                    // +0xf0, RandomTrack() steps when set
    int mUnknownf4;                  // +0xf4
    std::vector<bool> mPlayedTracks; // +0xf8, one per playlist entry
    MetRemixRecord mRecord;          // +0x114
};

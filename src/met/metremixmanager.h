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
struct MemcardConnectState;

/**
 * Manager of the remix catalogue. It also presents itself as a dialogue screen.
 *
 * Its RTTI descriptor is at `0x008efc50`. It has three public non-virtual bases at fixed offsets,
 * MetScreen at `+0x00`, MemcardUser at `+140`, and AsyncCallback at `+144`. Its asserts
 * record its file, `MetRemixManager.cpp`, at `0x00807bb0`. It is one of only two classes in the
 * subsystem whose file name remains in the image.
 *
 * The object is 0x14c bytes. The factory at `0x00361020` requests exactly that many with the tag
 * `MsgSink`. The figure agrees with the recovered member map below, whose last
 * member ends at `+0x14b`. The tag is MsgSink's rather than this class's, because MsgSink is the
 * base that declares `operator new`.
 *
 * The rest of the game resolves the one instance through shared(). shared() resolves it lazily by
 * handing the registry key `MetRemixManager` to MetScreen::FindScreenByName(). The manager is a
 * registered screen rather than a separately constructed singleton.
 *
 * Three vtables belong to the class, the 39-entry primary at `0x00807de0`, the 21-entry
 * MemcardUser table at `0x00807d30` that adjusts `this` by `-140`, and the three-entry
 * AsyncCallback table at `0x00807d10` that adjusts it by `-144`. The primary is the same length as
 * the MetScreen table. The class declares no new virtual.
 *
 * Five entries of the primary table differ from the MetScreen table. A comparison of the two
 * tables establishes them, not the title of each routine. They are 0 `0x00360788`, the
 * compiler-generated GetTypeInfo, 1 `0x00355070` the destructor, 5 `0x00361518`, 15 `0x003555c0`,
 * and 36 `0x00361550`. Slot 36 is a two-instruction bare return at an address the base table does
 * not include. The base's empty stubs are out-of-line definitions that every derived table shares.
 * A separate address is therefore an empty override in this class, declared below.
 *
 * The MemcardUser table overrides five slots at `0x003569d0`, `0x003573e8`, `0x003553e8`,
 * `0x00355ff0`, and `0x003563e0`, and the AsyncCallback table overrides its one slot at
 * `0x00358310`. All six are declared below with the spelling their base gives them.
 *
 * One of those spellings is now contradicted by the image. The MemcardUser slot 12 override opens
 * with a printf() of the literal at `0x00807a88`. The literal reads
 * ` in MetRemixManager::LoadRemixCB(). Return code `. The method is therefore named `LoadRemixCB`,
 * and MemcardUser declares slot 12 as `OnRemixLoaded`, a title that header records as inferred
 * from the task that reports through the slot rather than from any string. The declaration below
 * retains the base spelling, because an override that differs from its base by one letter is a new
 * virtual, and correcting the base is a change to MemcardUser and to every other class that
 * overrides the slot.
 *
 * The constructor at `0x00352b80` is member initialisation after the three vptr writes, and the
 * destructor at `0x00355070` is compiler-generated member destruction in reverse order. The member
 * list below reproduces both. The vector at `+0xf8` is the g++ 2.x `bit_vector`, whose two
 * iterators each include an empty base word. The vector therefore spans 0x1c bytes. The first
 * tree's teardown at `0x0035ef90` releases a vector of MetRemixRecord in each node, and the
 * second's at `0x003619a0` releases nothing. The two teardowns fix the value types of the trees.
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
     * @ghidraAddress NTSC-U/C: 0x00352b80
     * @ghidraAddress PAL: 0x0037ed28
     */
    MetRemixManager(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x00355070
     * @ghidraAddress PAL: 0x00381570
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
     * @ghidraAddress NTSC-U/C: 0x00361020
     * @ghidraAddress PAL: 0x0038e4e0
     */
    static MetRemixManager *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Return the one instance, resolving it through the screen registry on first use.
     *
     * @return The registered manager, or null before it is registered.
     * @ghidraAddress NTSC-U/C: 0x00361000
     * @ghidraAddress PAL: 0x0038e4c0
     */
    static MetRemixManager *shared();

    /**
     * Report the current remix record.
     *
     * The stats screens and MetSaveRemixScreen read it. The title is inferred.
     *
     * @return The record at `+0x114`.
     * @ghidraAddress NTSC-U/C: 0x00361558
     * @ghidraAddress PAL: 0x0038ea38
     */
    MetRemixRecord *GetRecord();

    /**
     * Replace the current remix record.
     *
     * MetRemixLoadScreen::OnExitFinished() is the caller. The title is inferred.
     *
     * @param record The record to copy.
     * @ghidraAddress NTSC-U/C: 0x00361560
     * @ghidraAddress PAL: 0x0038ea40
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
     * @ghidraAddress NTSC-U/C: 0x00359230
     * @ghidraAddress PAL: 0x00386388
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
     * @ghidraAddress NTSC-U/C: 0x003612c0
     * @ghidraAddress PAL: 0x0038e7a0
     */
    MetRemixRecord *LookupRemix(const HxStr &name);

    /**
     * Drop the playlist entries the catalogue no longer knows.
     *
     * MetJukeboxTopButtonsScreen is the caller. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x003612a0
     * @ghidraAddress PAL: 0x0038e780
     */
    void PrunePlayList();

    /**
     * Start loading the current playlist track.
     *
     * The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x003612e0
     * @ghidraAddress PAL: 0x0038e7c0
     */
    void PlayCurrentTrack();

    /**
     * Step to the previous playlist track, or to a random one in shuffle mode.
     *
     * The step stops at the first track. The image records no caller. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x00361300
     * @ghidraAddress PAL: 0x0038e7e0
     */
    void PreviousTrack();

    /**
     * Step to the next playlist track, or to a random one in shuffle mode.
     *
     * The step wraps from the last track to the first. Inline. StartLoadedRemix() expands it, and
     * the out-of-line copy has no caller. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x00361358
     * @ghidraAddress PAL: 0x0038e838
     */
    inline void NextTrack();

    /**
     * Pick a random track that has not played since the last reset.
     *
     * Every track is marked unplayed again once all have played. The title and the member it
     * sets are attested by the log line the routine writes.
     *
     * @ghidraAddress NTSC-U/C: 0x0035a7e0
     * @ghidraAddress PAL: 0x00387a40
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
     * @ghidraAddress NTSC-U/C: 0x003593d0
     * @ghidraAddress PAL: 0x00386528
     */
    void StartPlayList(const std::vector<HxStr> &returnScreens, int nShuffle);

    /**
     * Save the playlist to the first memory-card slot.
     *
     * Records the caller's screens, raises the `mem_save` warning with the slot's name formatted
     * into its text, and queues a SaveJukeboxPlayListMCT that reports to this manager. The European
     * release identifies the card in port 1, or `1` when GlobalSettings records another card.
     * MetJukeboxEditPlaylistScreenDone slot 36 and OnMsgScreenDismissed() call it. The title is
     * inferred.
     *
     * @param returnScreens The screens the caller wants restored.
     * @ghidraAddress NTSC-U/C: 0x00356508
     * @ghidraAddress PAL: 0x00382fc8
     */
    void SavePlayList(const std::vector<HxStr> &returnScreens);

    /**
     * Raise the load warning and start loading one remix.
     *
     * The warning text identifies the factory set or the first memory-card slot, and its wording
     * follows the play mode. mAfterLoadAction is set to exit the dialogue once the load completes,
     * both screen lists are replaced, and the load runs through LoadRemix().
     * MetRemixLoadScreen::OnExitFinished() is the caller. The title is inferred.
     *
     * @param returnScreens The screens mReturnScreens receives.
     * @param restoreScreens The screens mRestoreScreens receives.
     * @param record The remix to load.
     * @param nFactory Non-zero for a factory remix.
     * @ghidraAddress NTSC-U/C: 0x003548e8
     * @ghidraAddress PAL: 0x00380cd8
     */
    void BeginRemixLoad(const std::vector<HxStr> &returnScreens,
                        const std::vector<HxStr> &restoreScreens,
                        const MetRemixRecord &record,
                        int nFactory);

    /**
     * Rebuild the catalogue from the factory set and the given memory-card slots.
     *
     * Raises the `mem_load` warning identifying the sources, empties both trees, starts the factory
     * index read for the factory entry and a ListRemixesMCT for every card slot, empties the
     * playlist without releasing its entries, and optionally queues a playlist load. The two
     * remix screens, MetMemCardTypeScreen, and MetRemixDelScreen call it. The title is inferred.
     *
     * @param returnScreens The screens mReturnScreens receives.
     * @param slots The locations to list, the factory set having a port and slot of -1.
     * @param bLoadPlayList Non-zero to also load the playlist from the first slot.
     * @ghidraAddress NTSC-U/C: 0x00353350
     * @ghidraAddress PAL: 0x0037f5a8
     */
    void ListRemixes(const std::vector<HxStr> &returnScreens,
                     std::vector<MemcardConnectState> slots,
                     int bLoadPlayList);

    /**
     * Clear the jukebox flag in the game parameters and rewind the playlist.
     *
     * The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x0035a6b0
     * @ghidraAddress PAL: 0x003878a0
     */
    void LeaveJukeboxMode();

    /**
     * Hide the dialogue view instead of showing it. Slot 5.
     *
     * The whole body is one call. It dispatches Rnd::Drawable::SetShowing() with a zero argument on
     * the Drawable subobject of MetScreen::mView, at `+0x18` within the view, the
     * same subobject MetScreen::Draw() forwards to. The view is dereferenced with no null check,
     * and nothing is shown. The manager registers as a screen only to receive messages.
     *
     * @ghidraAddress NTSC-U/C: 0x00361518
     * @ghidraAddress PAL: 0x0038e9f8
     */
    virtual void EnterAndShow();

    /**
     * Slot 36, overridden empty.
     *
     * @ghidraAddress NTSC-U/C: 0x00361550
     * @ghidraAddress PAL: 0x0038ea30
     */
    virtual void OnExitFinished();

    /**
     * Act on the choice the user made in one of the manager's dialogues. Slot 15.
     *
     * The dialogue name selects the reaction. Most dismissals restore the screens in
     * mReturnScreens. A failed or unformatted save retries the playlist save on the first choice,
     * the format check queues a format on the second, a failed remix load abandons the jukebox
     * game, and a finished format retries the save. In the European release the first choice of
     * the format check raises `save_fail_no_format` instead, whose choices act as those of
     * `format_fail` do.
     *
     * @param name The dialogue the screen requested, as the message screen reports it back.
     * @param nChoice Which of the dialogue's buttons the user chose, counted from zero.
     * @ghidraAddress NTSC-U/C: 0x003555c0
     * @ghidraAddress PAL: 0x00381b20
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Act on the format the card reported. MemcardUser slot 5.
     *
     * A zero status and a status of 13 both raise the `mem_format_done` dialogue, with the
     * `format_success` and `format_already` texts respectively, and make it the active panel. Any
     * other status raises `format_fail` with retry and continue buttons. The port and slot argument
     * is not read.
     *
     * @param nPortSlot Which card port and slot reported. The body does not read it.
     * @param nStatus Zero on success, and 13 for the one failure the second path covers.
     * @ghidraAddress NTSC-U/C: 0x003569d0
     * @ghidraAddress PAL: 0x00383638
     */
    virtual void OnCardFormatted(int nPortSlot, int nStatus);

    /**
     * Act on the playlist save the card reported. MemcardUser slot 10.
     *
     * A zero status exits `MetMsgScreen`. An unformatted card raises `mem_format_check`, a missing
     * card or a full one raises `playlist_save_failed_tryagain`, and any other status raises
     * `playlist_save_failed`. The port and slot argument is not read. The European release calls
     * the card `1` in the missing-card text, and gives the full-card text nKilobytes.
     *
     * @param nPortSlot Which card port and slot reported. The body does not read it.
     * @param nStatus Zero on success.
     * @param nKilobytes The kilobytes the card lacked. European release only.
     * @ghidraAddress NTSC-U/C: 0x003573e8
     * @ghidraAddress PAL: 0x003841e0
     */
#ifdef VIDEO_STANDARD_PAL
    virtual void OnJukeboxPlayListSaved(int nPortSlot, int nStatus, int nKilobytes);
#else
    virtual void OnJukeboxPlayListSaved(int nPortSlot, int nStatus);
#endif

    /**
     * Record the remixes one card slot reported. MemcardUser slot 11.
     *
     * Records the status under the port and slot in mListStatus and counts one more listing in
     * mListingsDone. Once mListingsDone equals mListingsExpected while mPlayListReady is set, the
     * `MetMsgScreen` dialogue is exited. The status branches against 0 and 3 lead to the same code.
     *
     * @param nPortSlot Which card port and slot reported.
     * @param nStatus Zero on success.
     * @ghidraAddress NTSC-U/C: 0x003553e8
     * @ghidraAddress PAL: 0x00381918
     */
    virtual void OnRemixesListed(int nPortSlot, int nStatus);

    /**
     * Act on the remix load the card reported. MemcardUser slot 12.
     *
     * The image identifies this method as `LoadRemixCB`, and the class documentation records why
     * the declaration retains the base spelling.
     *
     * It opens by writing the status to the log through the literal at `0x00807a88`. A zero status
     * starts the remix or exits `MetMsgScreen`, as mAfterLoadAction selects. A non-zero status logs
     * `Failed to load remix from memory card slot %i.` and raises the `remix_load_failed`
     * dialogue. The port and slot argument is used only by the diagnostic message. The
     * European release shows `load_fail_jukebox` instead of `load_fail` when mAfterLoadAction
     * selects starting the remix.
     *
     * @param nPortSlot Which card port and slot reported. Only the failure message formats it.
     * @param nStatus Zero on success.
     * @ghidraAddress NTSC-U/C: 0x00355ff0
     * @ghidraAddress PAL: 0x00382920
     */
    virtual void OnRemixLoaded(int nPortSlot, int nStatus);

    /**
     * Act on the playlist load the card reported. MemcardUser slot 15.
     *
     * Sets mPlayListReady on every path, and exits `MetMsgScreen` when the two counters
     * mListingsDone and mListingsExpected agree. The zero and non-zero status branches lead to the
     * same code. The port and slot argument is not read.
     *
     * @param nPortSlot Which card port and slot reported. The body does not read it.
     * @param nStatus Zero on success.
     * @ghidraAddress NTSC-U/C: 0x003563e0
     * @ghidraAddress PAL: 0x00382e70
     */
    virtual void OnJukeboxPlayListLoaded(int nPortSlot, int nStatus);

    /**
     * Act on the asynchronous read the file layer finished. AsyncCallback slot 2.
     *
     * A completed remix read is copied into the reset log and started, or its dialogue exited, as
     * mAfterLoadAction selects. A completed index read is parsed into records filed under the slot
     * key mIndexSlot, the buffer is released, and the listing is counted as OnRemixesListed()
     * counts it. Any other handle is ignored.
     *
     * @param nHandle The request the completion belongs to, compared against the word at `+0xe4`.
     * @param nFile The file the request read from.
     * @param pBuffer The bytes the request read.
     * @param nLength How many bytes arrived.
     * @param nStatus Zero on success.
     * @ghidraAddress NTSC-U/C: 0x00358310
     * @ghidraAddress PAL: 0x00385380
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);

    /**
     * The key of the factory remixes in mRemixes. Card slots use port-and-slot keys.
     *
     * Public because MetRemixLoadScreen indexes mRemixes with it when the factory catalogue is
     * chosen.
     */
    static constexpr int kFactorySlot = -1;

private:
    /**
     * The shared manager, looked up first through CacheSharedInstance().
     *
     * @return The recorded manager.
     * @ghidraAddress NTSC-U/C: 0x003610a8
     * @ghidraAddress PAL: 0x0038e568
     */
    static MetRemixManager *ResolveSharedInstance();
    /**
     * Find the screen with the registry name and record it as the shared manager, unless one is
     * already recorded.
     *
     * @ghidraAddress NTSC-U/C: 0x00361210
     * @ghidraAddress PAL: 0x0038e6d0
     */
    static void CacheSharedInstance();
    /**
     * Starts reading the remix index file, recording the request in mIndexRequest.
     *
     * @ghidraAddress NTSC-U/C: 0x00357f80
     * @ghidraAddress PAL: 0x00384f10
     */
    void LoadIndex();
    /**
     * Starts reading one remix file, recording the request in mRemixRequest.
     *
     * @ghidraAddress NTSC-U/C: 0x003580f0
     * @ghidraAddress PAL: 0x003850d8
     */
    void LoadRemixFile(const HxStr &fileName);
    // NTSC-U/C: 0x00361418, PAL: 0x0038e8f8
    // A factory remix is read from its file and any other through the memory card.
    // LoadCurrentTrack() expands it inline.
    inline void LoadRemix(const MetRemixRecord &record, int nFactory);
    // NTSC-U/C: 0x00361480, PAL: 0x0038e960
    void LoadCurrentTrack();
    // NTSC-U/C: 0x0035abf8, PAL: 0x00387e58
    // Starts a jukebox game on the current track's remix in a random arena, burns the recorded
    // appearances, steps the playlist, and brings up MetLoadGameScreen. Both remix-load
    // completions call it. The title is inferred.
    void StartLoadedRemix();
    // Saves the playlist again with a copy of mReturnScreens as the return screens.
    // OnMsgScreenDismissed() expands it at each retry.
    inline void RetrySavePlayList();
    /**
     * Clamps into zero through the track count.
     *
     * The range admits one past the end.
     *
     * @ghidraAddress NTSC-U/C: 0x003613d8
     * @ghidraAddress PAL: 0x0038e8b8
     */
    void SetCurrentTrack(int nTrack);
    // NTSC-U/C: 0x003610d0, PAL: 0x0038e590
    // Pushes every screen listed in mRestoreScreens and activates the first.
    void PushRestoreScreens();
    // NTSC-U/C: 0x00361170, PAL: 0x0038e630
    // Pushes every screen listed in mReturnScreens and activates the first.
    void PushReturnScreens();

    // NTSC-U/C: 0x006c1110, PAL: 0x00704108
    static MetRemixManager *sInstance;

public:
    /**
     * The remix lists, keyed by card slot, with -1 for the factory remixes.
     *
     * Public because MetJukeboxBaseScreen::BindLists() and EnterAndShow() index it directly
     * through an inline operator[], and the image has no accessor. +0x94
     */
    std::map<int, std::vector<MetRemixRecord>> mRemixes;

    /**
     * The listing status of each card, keyed by port and slot.
     *
     * Public because MetRemixDelScreen::EnterAndShow() indexes it directly through an inline
     * operator[], and the image has no accessor. +0xa0
     */
    std::map<int, int> mListStatus;

private:
    // The screens a dismissed manager dialogue brings back. +0xac
    std::vector<HxStr> mReturnScreens;
    // The screens restored after a jukebox track or a failed remix load. +0xb8
    std::vector<HxStr> mRestoreScreens;

public:
    /**
     * The jukebox playlist.
     *
     * Public because MetJukeboxBaseScreen::BindLists() and EnterAndShow() take its address, and
     * MetJukeboxEditPlaylistScreenDone::OnRepeatingSoundFinished() reads its entry count, with no
     * accessor in the image. +0xc4
     */
    JukeboxPlayList mPlayList;

private:
    int mListingsDone;     // +0xd4, listings received since ListRemixes()
    int mListingsExpected; // +0xd8, sources ListRemixes() requested
    int mPlayListReady;    // +0xdc, starts at one, clear while a playlist load is queued
    int mIndexRequest;     // +0xe0
    int mRemixRequest;     // +0xe4, matched by Done()
    // The mRemixes key Done() files the index under. Starts at -1.
    int mIndexSlot;                  // +0xe8
    int mCurrentPlaylistTrack;       // +0xec
    int mShuffle;                    // +0xf0, RandomTrack() steps when set
    int mAfterLoadAction;            // +0xf4, start the remix or exit the dialogue after a load
    std::vector<bool> mPlayedTracks; // +0xf8, one per playlist entry
    MetRemixRecord mRecord;          // +0x114
};

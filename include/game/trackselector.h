#pragma once

#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/nullplayer.h"
#include "game/player.h"
#include "msg/bumppacket.h"
#include "msg/message.h"

class PhraseMuffedMsg;
class RemoteTrackSelectMsg;
class RotLeftMsg;
class RotRightMsg;

/** Channels the selector tracks. The constructor writes this count into the object. */
constexpr int kTrackSelectorChannelCount = 8;

/**
 * Slots reserved per channel.
 *
 * The column stride the constructor and every accessor apply is 0x10 bytes over four-byte
 * elements, so a column stores four. The fill loop's bound is the roster size rather than this
 * constant, and a fifth player would therefore write past its column. Every caller in the image
 * passes at most four players.
 */
constexpr int kTrackSelectorSlotCount = 4;

/**
 * Grid of which players occupy which track channel.
 *
 * `13TrackSelector` in the RTTI descriptor at `0x008ef688`, over MsgSink at offset 0 and MsgSource
 * at offset 4. Two tables belong to it, the primary at `0x007d37d8` with four entries and the
 * MsgSource subobject table at `0x007d37b0` with four and a `-4` adjustment on every entry. Both
 * run to the same length as their bases, so the class introduces no virtual of its own and
 * overrides only the destructor and HandleMessage().
 *
 * The grid is `kTrackSelectorChannelCount` columns of `kTrackSelectorSlotCount` player pointers at
 * `+0x20`, which makes the object 0xa0 bytes. An unoccupied slot stores the address of the
 * file-scope NullPlayer at `0x0066f930` rather than a null pointer, which is why every comparison
 * here is against that object rather than against zero. The Player translation unit's static
 * initialiser at `0x00132618` builds it, alongside the `IDable<Player>` table at `0x0066f920`.
 *
 * A player's own channel comes from Player::Slot4(), primary table slot 4, and the constructor
 * skips a player whose answer is -1. Player::Slot2() reports the payload word the message paths
 * check before acting, and Player::Slot5() drives the walk RebuildChannelGrid() performs.
 *
 * An earlier pass titled this class's routines for a renderer light manager, and the RTTI harvest
 * still records `RndLightManager__GetTypeInfo` as this descriptor's accessor. No descriptor among
 * the 574 in the image bears that title. The accessor is at `0x0013f0f0` and guards on the
 * descriptor at `0x008ef688`, which is what settles the name. The three private helpers below
 * retain the titles that pass gave them, because a rename would break every reference to them.
 *
 * RemoveLightFromColumn(), InsertLightForDrawable(), and RebuildChannelGrid() each build a message
 * field by field on the stack and send it, so the message classes carry public fields rather than
 * a constructor.
 *
 * The unit registers SelfTest() with TestRegistry under the name `TrackSelector`.
 *
 * Every data member is private. The five message paths and the three rebinding helpers are the
 * only code in the image that reads one, and each is a member of this class.
 */
class TrackSelector : public MsgSink, public MsgSource {
public:
    /**
     * Build the grid over a roster of players.
     *
     * Every slot of every column starts as the NullPlayer, and each player whose Player::Slot4()
     * reports a channel other than -1 is then inserted into that channel with a zero payload.
     *
     * @param players The roster. Only its size and its elements are read.
     * @ghidraAddress 0x0013b250
     */
    TrackSelector(const std::vector<Player *> &players);

    /**
     * @ghidraAddress 0x0013f020
     */
    virtual ~TrackSelector();

    /**
     * Act on a message.
     *
     * Slot 3 of the MsgSink table. A RotLeftMsg rotates the addressed player one channel down and
     * a RotRightMsg one channel up, both only when Player::Slot2() reports a value other than -1.
     * A BumpPacket rebuilds a column, a RemoteTrackSelectMsg rebinds one channel, and a
     * PhraseMuffedMsg is forwarded to the sink the addressed player provides. Every other message
     * is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0013b868
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Exercise the grid against four stand-in players.
     *
     * The routine builds four LocalPlayer objects with the colour name `null`, constructs a grid
     * over them, registers the selector with each, and then runs a fixed sequence of rebinds and
     * channel queries whose results it discards. It destroys the grid and the roster vector but
     * not the players. Nothing in the shipped game calls it apart from the test registry.
     *
     * @return Always 1.
     * @ghidraAddress 0x0013ba08
     */
    static int SelfTest();

    /**
     * Run SelfTest() in the shape TestRegistry::TestFunc requires, discarding its result.
     *
     * The unit's static initialiser registers it.
     *
     * @ghidraAddress 0x0013f8e8
     */
    static void RunSelfTest();

private:
    // Close the gap one player occupies in a channel's column by shifting every slot above it
    // down, filling the last with the NullPlayer, and announcing each move with a TrackSelectMsg
    // whose payload is the channel, the slot moved into, nPayload, and the player moved in. A
    // channel of -1 is ignored.
    // 0x0013b480
    void RemoveLightFromColumn(Player *pPlayer, int nChannel, int nPayload);

    // Store a player in the first slot of a channel's column that still holds the NullPlayer and
    // announce it with a TrackSelectMsg whose payload is the channel, that slot, nPayload, and the
    // player. A full column is ignored.
    // 0x0013b5e8
    void InsertLightForDrawable(Player *pPlayer, int nChannel, int nPayload);

    // Rebind a column from a BumpPacket. When the packet's player reports a step through
    // Player::Slot5(), announce a bumper with a DeployedPowerupMsg, rebind the head of the column
    // for as long as the player keeps reporting one, and mark the packet handled. The position is
    // the packet's bar in ticks, clamped to the finite range.
    // 0x0013b6a8
    int RebuildChannelGrid(BumpPacket *pPacket);

    // The four handlers below are inline, and HandleMessage() expands each. The addresses are
    // their uncalled out-of-line copies.

    // 0x0013f5a0
    // Moves the addressed player one channel down when it has an input slot.
    void OnRotLeft(RotLeftMsg *pMsg);

    // 0x0013f608
    // Moves the addressed player one channel up when it has an input slot.
    void OnRotRight(RotRightMsg *pMsg);

    // 0x0013f670
    // Passes a muff to the player's own sink when the player has an input slot.
    void OnPhraseMuffed(PhraseMuffedMsg *pMsg);

    // 0x0013f6d8
    // Moves the player from its own channel to the one the message selects.
    void OnRemoteTrackSelect(RemoteTrackSelectMsg *pMsg);

    // Move a player from one channel to another.
    // 0x0013f748
    void RebindLightColumn(Player *pPlayer, int nFromChannel, int nToChannel, int nPayload);

    // Rebind a channel unless the player already occupies its first slot and its second slot is
    // unoccupied.
    // 0x0013f7a8
    void RebindLightIfChanged(Player *pPlayer, int nChannel, int nPayload);

    // Move a player the given number of channels from its own, wrapping at the channel count.
    // 0x0013f840
    int AddLightToChannel(Player *pPlayer, int nPayload, int nDelta);

    int mChannelCount; // +0x18, always kTrackSelectorChannelCount
    int mSlotCount;    // +0x1c, the roster size the constructor measured
    Player *mGrid[kTrackSelectorChannelCount][kTrackSelectorSlotCount]; // +0x20
};

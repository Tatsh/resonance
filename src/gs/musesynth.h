#pragma once

#include <list>

#include "app/msgsink.h"
#include "gs/msgsplitter.h"
#include "gs/museparent.h"
#include "gs/museplayer.h"
#include "msg/message.h"
#include "sch/tickclock.h"

class StdMidiMsg;
class SustainNoteMsg;
class SynthSustainer;

/**
 * Owner of the players that are sounding, and the sink that starts them.
 *
 * `9MuseSynth` in the RTTI descriptor at `0x00902290`, with MsgSink at offset 0 and MuseParent at
 * offset 4. The object is 0x30 bytes, which MultiMusePlayer's MusePlayer base at offset 48
 * confirms. Its primary table is at `0x007e0008` and its MuseParent table at `0x007dffe8`.
 *
 * It is a MsgSink that creates a player per message. A NoteMsg creates a 0x20-byte NotePlayer
 * through the constructor at `0x001b4328`, a MultiMuseMsg creates a MultiMusePlayer, and an
 * AllNotesOffMsg releases every player. A StdMidiMsg or a SustainNoteMsg is instead forwarded
 * straight to mOutput, so a message that needs no scheduling bypasses the player list. Every other
 * message is discarded.
 *
 * Every player it creates is started against mOutput, which addresses its own embedded
 * MsgSplitter, or the sustainer CreateSustainer() places in front of it. AddSink() registers a
 * sink with that splitter, so a caller that wants the sound registers once here rather than with
 * each player.
 *
 * The name comes from the RTTI descriptor and is not the invented `Rnd::LightMsgSplitter` that the
 * type-function harvest recorded for the accessor. Neither is this class a light manager: the two
 * members previously titled AddDirectionalLight() and AddPointLight() are the NoteMsg and the
 * MultiMuseMsg handler, and ClearLights() is the AllNotesOffMsg handler.
 */
class MuseSynth : public MsgSink, public MuseParent {
public:
    /**
     * @param pClock The clock every player it creates is scheduled against.
     * @ghidraAddress 0x001aa4d8
     */
    MuseSynth(Sch::TickClock *pClock);

    /**
     * Release every player and then the splitter.
     *
     * @ghidraAddress 0x001aa5d8
     */
    virtual ~MuseSynth();

    /**
     * Register a sink with the splitter every player sends to.
     *
     * @param pSink The sink to register.
     * @ghidraAddress 0x001ab038
     */
    void AddSink(MsgSink *pSink);

    /**
     * Build the sustain filter this synthesiser feeds through.
     *
     * The routine allocates a SynthSustainer under the tag `MsgSink`, stores it at both `+0x24`
     * and `+0x28`, and points its downstream sink at this object's `+0x0c`. AxingSTG is the only
     * caller. The title is inferred from that body.
     *
     * @ghidraAddress 0x001aafb0
     */
    void CreateSustainer();

    /**
     * @ghidraAddress 0x001aa908
     */
    virtual void RetainOnly(MusePlayer *pPlayer);

    /**
     * @ghidraAddress 0x001aa9f0
     */
    virtual void PlayerFinished(MusePlayer *pPlayer);

    /**
     * Report whether any player is sounding.
     *
     * The body counts the nodes of mPlayers. The image has no caller. The title is inferred.
     *
     * @return Non-zero while mPlayers is not empty.
     * @ghidraAddress 0x001aaf68
     */
    int HasPlayers();

    /**
     * Stop and release every player.
     *
     * @ghidraAddress 0x001ab0d8
     */
    void ReleaseAllPlayers();

protected:
    /**
     * Start a note player for one NoteMsg.
     *
     * Creates a 0x20-byte NotePlayer from the message's three payload bytes and its word at
     * `+0x0c`, stores it, and starts it against mOutput.
     *
     * @param pMsg The NoteMsg.
     * @ghidraAddress 0x001aa690
     */
    void StartNotePlayer(Message *pMsg);

    /**
     * Start a MultiMusePlayer for one MultiMuseMsg.
     *
     * Creates a 0x50-byte MultiMusePlayer over the message's sequence, stores its MusePlayer
     * subobject, and starts it against mOutput. A null player is stored as a null pointer rather
     * than as the fixed subobject displacement, which is the compiler's own null check on the
     * base-class conversion.
     *
     * @param pMsg The MultiMuseMsg.
     * @ghidraAddress 0x001aa7c0
     */
    void StartMultiMusePlayer(Message *pMsg);

    /**
     * Act on a message.
     *
     * Primary table slot 3. MultiMusePlayer retains this body unchanged.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001ab170
     */
    virtual void HandleMessage(Message *pMsg);

    // The out-of-line copy of the AllNotesOffMsg branch HandleMessage() expands inline. The message
    // is not read.
    // 0x001ab0b8
    void OnAllNotesOff();

    // The out-of-line copy of the StdMidiMsg branch HandleMessage() expands inline.
    // 0x001ab058
    void OnStdMidi(StdMidiMsg *pMsg);

    // The out-of-line copy of the SustainNoteMsg branch HandleMessage() expands inline. Its body
    // compiles to the same bytes as OnStdMidi().
    // 0x001ab088
    void OnSustainNote(SustainNoteMsg *pMsg);

    // The clock every player is scheduled against.
    Sch::TickClock *mClock; // +0x08
    // Every player this object created reports back to it and sends through it.
    MsgSplitter mSplitter; // +0x0c
    // Null until CreateSustainer() stores the new SynthSustainer here.
    SynthSustainer *mSustainer; // +0x24
    // The sink every player is started against. It addresses mSplitter until CreateSustainer()
    // points it at the sustainer, whose own output is mSplitter.
    MsgSink *mOutput; // +0x28
    // Every player currently sounding. A four-byte element places the value at +0x08 of a 16-byte
    // node.
    std::list<MusePlayer *> mPlayers; // +0x2c
};

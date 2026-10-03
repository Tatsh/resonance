#pragma once

#include "app/msgsink.h"
#include "mid/tickobj.h"

class Message;
class MuseMsg;

/**
 * Collector of a MIDI channel's controller state, for replaying it before playback starts.
 *
 * It has MsgSink as its one base. Its unit spans `0x001a6488` through `0x001a6960`.
 * BGTrackGraph::BuildSequencer() builds one on its stack, feeds it every bar of the track, and has
 * it replay what it collected into the synthesiser.
 *
 * Every byte of state starts at kUnset, which Replay() treats as nothing collected.
 */
class MidiChase : public MsgSink {
public:
    /** The value every collected byte starts at. */
    static constexpr unsigned char kUnset = 0xff;

    /** The number of MIDI controllers. */
    static constexpr int kControllerCount = 128;

    /**
     * Start with nothing collected.
     *
     * @ghidraAddress NTSC-U/C: 0x001a6888
     * @ghidraAddress PAL: 0x001ac5f0
     */
    MidiChase();

    /**
     * @ghidraAddress NTSC-U/C: 0x001a67d8
     * @ghidraAddress PAL: 0x001ac540
     */
    virtual ~MidiChase();

    /**
     * Record the channel, and the state a controller change, a program change, or a pitch bend
     * sets.
     *
     * Every other message is ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001a6660
     * @ghidraAddress PAL: 0x001ac3c8
     */
    virtual void DispatchPriv(Message *pMsg);

    /**
     * Pass every message of a range to MsgSink::Dispatch().
     *
     * The title is inferred.
     *
     * @param pBegin The first message.
     * @param pEnd One past the last message.
     * @ghidraAddress NTSC-U/C: 0x001a6900
     * @ghidraAddress PAL: 0x001ac668
     */
    void HandleRange(const TickObj<MuseMsg *> *pBegin, const TickObj<MuseMsg *> *pEnd);

    /**
     * Send the collected state to a sink.
     *
     * Sends a controller change for every collected controller in order, then the program change,
     * then the pitch bend, each only when collected, all on the recorded channel at kMBTInfinity.
     * The title is inferred.
     *
     * @param pSink The sink.
     * @ghidraAddress NTSC-U/C: 0x001a6488
     * @ghidraAddress PAL: 0x001ac1f0
     */
    void Replay(MsgSink *pSink);

private:
    unsigned char mChannel;                       // +0x04
    unsigned char mProgram;                       // +0x05
    unsigned char mBendLow;                       // +0x06
    unsigned char mBendHigh;                      // +0x07
    unsigned char mControllers[kControllerCount]; // +0x08
};

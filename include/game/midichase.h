#pragma once

#include "app/msgsink.h"
#include "mid/tickobj.h"

class Message;
class MuseMsg;

/**
 * Collector of a MIDI channel's controller state, for replaying it before playback starts.
 *
 * `9MidiChase` in the RTTI descriptor, with MsgSink as its one base. Its unit spans `0x001a6488`
 * through `0x001a6960`. BGTrackGraph::BuildSequencer() builds one on its stack, feeds it every
 * bar of the track, and has it replay what it collected into the synthesiser.
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
     * @ghidraAddress 0x001a6888
     */
    MidiChase();

    /** @ghidraAddress 0x001a67d8 */
    virtual ~MidiChase();

    /**
     * Record the channel, and the state a controller change, a program change, or a pitch bend
     * sets.
     *
     * Every other message is ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a6660
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Pass every message of a range to MsgSink::Handle().
     *
     * The title is inferred.
     *
     * @param pBegin The first message.
     * @param pEnd One past the last message.
     * @ghidraAddress 0x001a6900
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
     * @ghidraAddress 0x001a6488
     */
    void Replay(MsgSink *pSink);

private:
    unsigned char mChannel;                       // +0x04
    unsigned char mProgram;                       // +0x05
    unsigned char mBendLow;                       // +0x06
    unsigned char mBendHigh;                      // +0x07
    unsigned char mControllers[kControllerCount]; // +0x08
};

#pragma once

#include "app/msgsink.h"
#include "msg/message.h"

class StdMidiMsg;

/**
 * Abstract base of the game's sound output.
 *
 * `5Synth` in the RTTI descriptor at `0x008ef430`, with MsgSink as its one base. Its own vtable is
 * at `0x007d2ee0` and runs seventeen entries, slots 0 through 16. Slot 9 addresses the shared
 * pure-virtual stub at `0x005381a8`, which is what makes the class abstract. Two classes implement
 * it. Ps2HardSynth drives the hardware, and the file-local `Synth::Setup::NullSynth` at
 * `0x008eecf8` supplies an empty body for slot 9 at `0x0013a478` and inherits everything else. A
 * third file-local class, `Synth::Setup::SynthFade` at `0x008eebe8`, derives from TimeTask rather
 * than from this class.
 *
 * The interface is a MIDI port. Slot 9 moves one three-byte MIDI message to the hardware and every
 * other member of the class is expressed through it, which is what fixes the meaning of the whole
 * interface. AllNotesOff() sends controller 123 on all sixteen channels,
 * AllNotesOffExceptSfxChannel() sends the same on channels 0 through 14, SetChannelVolume() sends
 * controller 7, and SelectBank() sends controllers 0 and 32. Channel 15 is the sound-effect
 * channel, which the channel count of AllNotesOffExceptSfxChannel() and the separate channel-15
 * program selection in Ps2HardSynth's three bank loaders both attest.
 *
 * The bodies of slots 4 through 8 and 10 through 14 are two-instruction bare returns in this class
 * and are therefore its defaults rather than pure members. Slots 15 and 16 have real bodies here.
 *
 * Ps2HardSynth's table has eighteen slots and adds one of its own at slot 17, which is consistent
 * with the two implementations being siblings rather than one deriving from the other.
 *
 * The verb of slots 7, 10, 12, 13, and 14 is unrecovered, and of slots 4, 5, and 6 only the shape
 * is. No string in the image identifies any member of this class, so each of those declarations
 * records its table index and its address rather than an invented title. They are declared because
 * omitting a slot would shift every later index and state something false about the layout.
 *
 * Globals owns the single instance, created in Globals::CreateSynth() and destroyed in
 * Globals::Shutdown().
 */
class Synth : public MsgSink {
public:
    /**
     * @ghidraAddress 0x0013a398
     */
    virtual ~Synth();

    /**
     * Load one of the three bank sets.
     *
     * Table slot 4. The body here is empty. Ps2HardSynth's override at `0x003f47b8` unloads what
     * is resident, loads the pair of bank paths the configuration reports under codes 500 and 501
     * at placement 0, turns the reverb off, and selects the sound-effect programs. Which of the
     * three bank sets this is remains unrecovered, because the configuration codes are numeric and
     * no table in the image maps a code to a name.
     *
     * @ghidraAddress 0x0013a1d0
     */
    virtual void LoadBankSet4();

    /**
     * Load one of the three bank sets.
     *
     * Table slot 5. The body here is empty. Ps2HardSynth's override at `0x003f4960` loads the pair
     * under codes 507 and 508 at placement 1 and then resets.
     *
     * @ghidraAddress 0x0013a1d8
     */
    virtual void LoadBankSet5();

    /**
     * Load one of the three bank sets.
     *
     * Table slot 6. The body here is empty. Ps2HardSynth's override at `0x003f4af0` loads the pair
     * under codes 504 and 505 at the rotating placement, turns the reverb on, and then resets.
     *
     * @ghidraAddress 0x0013a1e0
     */
    virtual void LoadBankSet6();

    /**
     * Table slot 7, verb unrecovered.
     *
     * The body is empty here and no implementation overrides it, so nothing in the image reveals
     * what the member does or what it takes.
     *
     * @ghidraAddress 0x0013a1e8
     */
    virtual void Slot7();

    /**
     * Release every loaded bank.
     *
     * Table slot 8. The body here is empty. Ps2HardSynth's override at `0x003f6650` hands off to
     * the driver routine at `0x00464660`, which waits for the transfers in flight, releases the
     * streaming voice, and clears both stored bank paths.
     *
     * @ghidraAddress 0x0013a1f0
     */
    virtual void UnloadBanks();

    /**
     * Move one MIDI message to the hardware.
     *
     * Table slot 9, and the one pure member of the class. Every other member reaches the hardware
     * through this one. Ps2HardSynth's override at `0x003f66d8` masks each argument to a byte and
     * hands the three to the driver at `0x00464928`, which suppresses a program change or a bank
     * select that repeats what the channel already holds and packs the rest into one word for the
     * sound driver.
     *
     * @param nStatus The status byte, a command nibble ORed with the channel.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     */
    virtual void SendMidi(unsigned char nStatus, unsigned char nData1, unsigned char nData2) = 0;

    /**
     * Table slot 10, verb unrecovered.
     *
     * The body is empty here and no implementation overrides it.
     *
     * @ghidraAddress 0x0013a1f8
     */
    virtual void Slot10();

    /**
     * Select a bank on one channel.
     *
     * Table slot 11. The body here is empty. Ps2HardSynth's override at `0x003f65c8` sends
     * controller 0 with value 0 and then controller 32 with the bank, which is the two halves of a
     * MIDI bank select. That override performs no work unless the alternate bank set is resident.
     *
     * @param nChannel The channel.
     * @param nBank The bank, sent as the low half of the bank select.
     * @ghidraAddress 0x0013a200
     */
    virtual void SelectBank(unsigned char nChannel, unsigned char nBank);

    /**
     * Table slot 12, verb unrecovered.
     *
     * The body is empty here. Ps2HardSynth's override at `0x003f6700` submits sound-driver
     * selector 0x110 with the argument inverted, so the member and the driver command disagree on
     * which sense is the enabled one.
     *
     * @param bEnable The flag the override inverts.
     * @ghidraAddress 0x0013a208
     */
    virtual void Slot12(int bEnable);

    /**
     * Table slot 13, verb unrecovered.
     *
     * The body is empty here. Ps2HardSynth's override at `0x003f6740` submits sound-driver
     * selector 0x100 with the argument unchanged.
     *
     * @param nValue The value the override forwards.
     * @ghidraAddress 0x0013a210
     */
    virtual void Slot13(int nValue);

    /**
     * Table slot 14, verb unrecovered.
     *
     * The body is empty here. Ps2HardSynth's override at `0x003f6720` submits sound-driver
     * selector 0xf0 with the argument unchanged.
     *
     * @param nValue The value the override forwards.
     * @ghidraAddress 0x0013a218
     */
    virtual void Slot14(int nValue);

    /**
     * Silence every channel.
     *
     * Table slot 15. Sends controller 123 with value 0 on channels 0 through 15. Ps2HardSynth's
     * override at `0x003f4c50` calls this body first and then resets the rest of the channel state.
     *
     * @ghidraAddress 0x0013a220
     */
    virtual void AllNotesOff();

    /**
     * Silence every channel but the sound-effect channel.
     *
     * Table slot 16. The same as AllNotesOff() over channels 0 through 14, which retains whatever
     * is sounding on channel 15. Neither implementation overrides it.
     *
     * @ghidraAddress 0x0013a288
     */
    virtual void AllNotesOffExceptSfxChannel();

    /**
     * Set the volume of every channel.
     *
     * Sends controller 7 with the given value on channels 0 through 15. The member is not virtual
     * and occupies no table slot. Ps2HardSynth's AllNotesOff() override calls it with 100.
     *
     * @param nVolume The volume, 0 through 127.
     * @ghidraAddress 0x0013a2f0
     */
    void SetChannelVolume(unsigned char nVolume);

    /**
     * Put every channel into its starting state.
     *
     * On each of the sixteen channels, centres the pitch bend, resets every controller, silences
     * every note, sets the channel volume to 100, and sets the expression to 127. The member is
     * not virtual and nothing in the image calls it. It is the first routine of the unit, and the
     * anonymous namespace of the unit takes its `_GLOBAL_$N$Setup__5Synth` prefix from it, which is
     * what recovers the title.
     *
     * @ghidraAddress 0x00139fd0
     */
    void Setup();

    /**
     * Fade every channel's volume to silence over a duration.
     *
     * Creates a file-local SynthFade task on the watchdog time base, which samples a fade-out
     * Source every 100 milliseconds and sends the scaled volume to all sixteen channels, then
     * silences every note once the volume reaches zero. GrooveWorld::Exit() and
     * MetLoadGameScreen's slot 33 are the callers. The title is inferred.
     *
     * @param nDurationMs The length of the fade, in milliseconds.
     * @ghidraAddress 0x0013a480
     */
    void FadeOut(int nDurationMs);

protected:
    // 0x0013a360
    // Inline, and HandleMessage() expands it. Sends the message's three bytes through SendMidi().
    void OnStdMidi(StdMidiMsg *pMsg);

    /**
     * Act on a message.
     *
     * Table slot 3. Accepts StdMidiMsg alone and hands its three payload bytes to SendMidi().
     * Neither implementation overrides it.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0013a570
     */
    virtual void HandleMessage(Message *pMsg);
};

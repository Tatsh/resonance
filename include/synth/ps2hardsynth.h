#pragma once

#include "synth/midi_main.h"
#include "synth/synth.h"

/**
 * The game's hardware synthesiser.
 *
 * `12Ps2HardSynth` in the RTTI descriptor at `0x009021d0`, with Synth as its one base, so the
 * inherited MsgSink vptr sits at offset 0 and the class is 0xc bytes. Its vtable is at
 * `0x00814fc0` and has eighteen slots, one more than the seventeen Synth declares, so the class
 * adds a single virtual of its own at slot 17.
 *
 * It answers ten of Synth's slots and inherits slots 7, 10, and 16 unchanged. The class is thin.
 * Every override reaches the driver at `0x00461a88` through `0x00465200`, which owns the voices and
 * the sound banks, and the constructor brings that driver up through its initialiser at
 * `0x004647a8`.
 *
 * Slot 17 is the one member the class adds, and the three bank loaders are its only callers. It
 * prefixes both bank names with the device the host mode selects and hands the pair to
 * LoadSoundBank().
 *
 * Globals owns the single instance, created in Globals::CreateSynth() and destroyed in
 * Globals::Shutdown().
 */
class Ps2HardSynth : public Synth {
public:
    /**
     * Select the sound-effect bank and bring the synthesiser driver up.
     *
     * Sets mUseSfxBank and clears mAlternateBanksResident, then runs InitSynthDriver().
     * CreatePs2HardSynth() expands the same body in place, and the image lists no caller for this
     * out-of-line copy.
     *
     * @ghidraAddress 0x003f64e0
     */
    Ps2HardSynth();

    /**
     * @ghidraAddress 0x003f6670
     */
    virtual ~Ps2HardSynth();

    /**
     * @ghidraAddress 0x003f47b8
     */
    virtual void LoadBankSet4();

    /**
     * @ghidraAddress 0x003f4960
     */
    virtual void LoadBankSet5();

    /**
     * @ghidraAddress 0x003f4af0
     */
    virtual void LoadBankSet6();

    /**
     * @ghidraAddress 0x003f6650
     */
    virtual void UnloadBanks();

    /**
     * @ghidraAddress 0x003f66d8
     */
    virtual void SendMidi(unsigned char nStatus, unsigned char nData1, unsigned char nData2);

    /**
     * @ghidraAddress 0x003f65c8
     */
    virtual void SelectBank(unsigned char nChannel, unsigned char nBank);

    /**
     * @ghidraAddress 0x003f6700
     */
    virtual void Slot12(int bEnable);

    /**
     * @ghidraAddress 0x003f6740
     */
    virtual void Slot13(int nValue);

    /**
     * @ghidraAddress 0x003f6720
     */
    virtual void Slot14(int nValue);

    /**
     * @ghidraAddress 0x003f4c50
     */
    virtual void AllNotesOff();

    /**
     * Compose both bank paths and load the pair.
     *
     * Table slot 17, the one virtual this class adds. Prefixes each name with `host0:` when
     * GetHostMode() reports kHostModeHostOnly and with `cdrom0:` otherwise, then calls
     * LoadSoundBank() with the two composed paths. An empty composed path is passed as the shared
     * empty string rather than as a null pointer.
     *
     * @param bdName The BD bank name, without a device prefix.
     * @param hdName The HD bank name, without a device prefix.
     * @param nTag The tag both banks are recorded with.
     * @param nPlacement Where the pair goes in sound memory.
     * @ghidraAddress 0x003f4590
     */
    virtual void LoadBankPair(const HxStr &bdName, const HxStr &hdName, int nTag, int nPlacement);

    /**
     * Program the sound-effect channel.
     *
     * Sends bank select 0 and then bank 15, or bank 0 when mUseSfxBank is clear, on channel 15,
     * followed by program 16. LoadBankSet4(), LoadBankSet5(), and AllNotesOff() expand the same
     * three messages in place, and the image lists no caller for this out-of-line copy. The title
     * is inferred.
     *
     * @ghidraAddress 0x003f6540
     */
    void SelectSfxProgram();

private:
    // Selects the bank the sound-effect channel is switched to. The constructor sets it, and each
    // of the three bank loaders sets it again. Bank 15 is used while it is non-zero and bank 0
    // otherwise.
    int mUseSfxBank; // +0x04
    // Whether the alternate bank set is resident, read from configuration code 0x3a4 in
    // LoadBankSet6() and cleared by LoadBankSet4(). SelectBank() performs no work while it is
    // zero.
    int mAlternateBanksResident; // +0x08
};

/**
 * Create the hardware synthesiser.
 *
 * Takes 0xc bytes through MsgSink's allocation operator, under the tag `MsgSink` at `0x00814e90`,
 * and expands the Ps2HardSynth constructor in place.
 *
 * @return The new synthesiser.
 * @ghidraAddress 0x003f4db8
 */
Ps2HardSynth *CreatePs2HardSynth();

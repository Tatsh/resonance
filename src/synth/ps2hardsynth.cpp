#include "synth/ps2hardsynth.h"

#include "os/hostmode.h"
#include "os/hxstr.h"
#include "script/configquery.h"
#include "synth/midi_main.h"

namespace {

constexpr int kChannelCount = 16;
constexpr unsigned char kSfxChannel = 15;
constexpr unsigned char kMuseChannel = 14;
constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kStatusProgramChange = 0xc0;
constexpr unsigned char kControllerBankSelectMsb = 0;
constexpr unsigned char kControllerExpression = 11;
constexpr unsigned char kControllerBankSelectLsb = 32;
constexpr unsigned char kSfxBank = 15;
constexpr unsigned char kSfxProgram = 16;
constexpr unsigned char kMuseProgram = 15;
constexpr unsigned char kFullExpression = 127;
constexpr unsigned char kDefaultVolume = 100;

// Configuration codes each bank set reads its BD and HD name from. Only the codes are recovered;
// no table in the image maps a code to a name.
constexpr int kBankSet4BdCode = 500;
constexpr int kBankSet4HdCode = 501;
constexpr int kBankSet5BdCode = 507;
constexpr int kBankSet5HdCode = 508;
constexpr int kBankSet6BdCode = 504;
constexpr int kBankSet6HdCode = 505;
constexpr int kAlternateBanksCode = 0x3a4;

constexpr int kTagAllChannels = 15;
constexpr int kTagNone = 0;

constexpr int kPlacementFixed = 0;
constexpr int kPlacementFixedSecond = 1;
constexpr int kPlacementBuffer = 2;
constexpr int kPlacementRotating = 3;

} // namespace

// NTSC-U/C: 0x003f64e0, PAL: 0x0042ecc8
Ps2HardSynth::Ps2HardSynth() : mUseSfxBank(1), mAlternateBanksResident(0) {
    InitSynthDriver();
}

// NTSC-U/C: 0x003f6540, PAL: 0x0042ed28
void Ps2HardSynth::SelectSfxProgram() {
    PlayMidi(kStatusControlChange | kSfxChannel, kControllerBankSelectMsb, 0);
    PlayMidi(
        kStatusControlChange | kSfxChannel, kControllerBankSelectLsb, mUseSfxBank ? kSfxBank : 0);
    PlayMidi(kStatusProgramChange | kSfxChannel, kSfxProgram, 0);
}

// NTSC-U/C: 0x003f4db8, PAL: 0x0042d530
Ps2HardSynth *CreatePs2HardSynth() {
    return new Ps2HardSynth;
}

// NTSC-U/C: 0x003f6670, PAL: 0x0042ee58
Ps2HardSynth::~Ps2HardSynth() {
    ReleaseSoundBanks();
    ShutdownSynthDriver();
}

// NTSC-U/C: 0x003f47b8, PAL: 0x0042ce70
void Ps2HardSynth::LoadBankSet4() {
    UnloadBanks();

    HxStr bdName = QueryConfigString(kBankSet4BdCode, GetHostMode());
    HxStr hdName = QueryConfigString(kBankSet4HdCode, GetHostMode());
    LoadBankPair(bdName, hdName, kTagAllChannels, kPlacementFixed);

    ConfigureSpu2Effects(0);
    mAlternateBanksResident = 0;
    mUseSfxBank = 1;
    SelectSfxProgram();
}

// NTSC-U/C: 0x003f4960, PAL: 0x0042d058
void Ps2HardSynth::LoadBankSet5() {
    mUseSfxBank = 1;

    HxStr bdName = QueryConfigString(kBankSet5BdCode);
    HxStr hdName = QueryConfigString(kBankSet5HdCode);
    LoadBankPair(bdName, hdName, kTagAllChannels, kPlacementFixedSecond);

    SelectSfxProgram();
    WaitForBankTransfers();
    AllNotesOff();
}

// NTSC-U/C: 0x003f4af0, PAL: 0x0042d1e8
void Ps2HardSynth::LoadBankSet6() {
    mAlternateBanksResident = QueryConfigFlag(kAlternateBanksCode);

    HxStr bdName = QueryConfigString(kBankSet6BdCode, mAlternateBanksResident, GetHostMode());
    HxStr hdName = QueryConfigString(kBankSet6HdCode, mAlternateBanksResident, GetHostMode());
    LoadBankPair(
        bdName, hdName, kTagNone, mAlternateBanksResident ? kPlacementRotating : kPlacementBuffer);

    WaitForBankTransfers();
    ConfigureSpu2Effects(1);
    AllNotesOff();
}

// NTSC-U/C: 0x003f6650, PAL: 0x0042ee38
void Ps2HardSynth::UnloadBanks() {
    ReleaseSoundBanks();
}

// NTSC-U/C: 0x003f66d8, PAL: 0x0042eec0
void Ps2HardSynth::PlayMidi(unsigned char nStatus, unsigned char nData1, unsigned char nData2) {
    SendMidiToDriver(nStatus, nData1, nData2);
}

// NTSC-U/C: 0x003f65c8, PAL: 0x0042edb0
void Ps2HardSynth::SelectBank(unsigned char nChannel, unsigned char nBank) {
    if (mAlternateBanksResident == 0) {
        return;
    }

    const unsigned char nStatus = kStatusControlChange | nChannel;
    PlayMidi(nStatus, kControllerBankSelectMsb, 0);
    PlayMidi(nStatus, kControllerBankSelectLsb, nBank);
}

// NTSC-U/C: 0x003f6700, PAL: 0x0042eee8
void Ps2HardSynth::SetStereo(int bStereo) {
    SubmitDriverSetMono(bStereo ^ 1);
}

// NTSC-U/C: 0x003f6740, PAL: 0x0042ef28
void Ps2HardSynth::SetRemixMode(int bRemix) {
    SubmitDriverSetRemix(bRemix);
}

// NTSC-U/C: 0x003f6720, PAL: 0x0042ef08
void Ps2HardSynth::SetPaused(int bPaused) {
    SubmitDriverSetPaused(bPaused);
}

// NTSC-U/C: 0x003f4c50, PAL: 0x0042d3c8
void Ps2HardSynth::AllNotesOff() {
    Synth::AllNotesOff();

    for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
        const unsigned char nStatus = kStatusControlChange | nChannel;
        PlayMidi(nStatus, kControllerExpression, kFullExpression);
        PlayMidi(nStatus, kControllerBankSelectMsb, 0);
        PlayMidi(nStatus, kControllerBankSelectLsb, 0);
        PlayMidi(kStatusProgramChange | nChannel, 0, 0);
    }

    SetChannelVolume(kDefaultVolume);
    SelectSfxProgram();
    PlayMidi(kStatusProgramChange | kMuseChannel, kMuseProgram, 0);
}

// NTSC-U/C: 0x003f4590, PAL: 0x0042cbc8
void Ps2HardSynth::LoadBankPair(const HxStr &bdName,
                                const HxStr &hdName,
                                int nTag,
                                int nPlacement) {
    HxStr device(GetHostMode() == kHostModeHostOnly ? "host0:" : "cdrom0:");

    HxStr bdPath(device);
    bdPath += bdName;
    HxStr hdPath(device);
    hdPath += hdName;

    LoadSoundBank(bdPath.mStr != nullptr ? bdPath.mStr : g_szEmptyString,
                  hdPath.mStr != nullptr ? hdPath.mStr : g_szEmptyString,
                  nTag,
                  nPlacement);
}

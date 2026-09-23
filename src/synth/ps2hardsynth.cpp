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

// 0x003f64e0
Ps2HardSynth::Ps2HardSynth() : mUseSfxBank(1), mAlternateBanksResident(0) {
    InitSynthDriver();
}

// 0x003f6540
void Ps2HardSynth::SelectSfxProgram() {
    SendMidi(kStatusControlChange | kSfxChannel, kControllerBankSelectMsb, 0);
    SendMidi(
        kStatusControlChange | kSfxChannel, kControllerBankSelectLsb, mUseSfxBank ? kSfxBank : 0);
    SendMidi(kStatusProgramChange | kSfxChannel, kSfxProgram, 0);
}

// 0x003f4db8
Ps2HardSynth *CreatePs2HardSynth() {
    return new Ps2HardSynth;
}

// 0x003f6670
Ps2HardSynth::~Ps2HardSynth() {
    ReleaseSoundBanks();
    ShutdownSynthDriver();
}

// 0x003f47b8
void Ps2HardSynth::LoadBankSet4() {
    UnloadBanks();

    HxStr bdName;
    QueryConfigString(&bdName, kBankSet4BdCode, GetHostMode());
    HxStr hdName;
    QueryConfigString(&hdName, kBankSet4HdCode, GetHostMode());
    LoadBankPair(bdName, hdName, kTagAllChannels, kPlacementFixed);

    ConfigureSpu2Effects(0);
    mAlternateBanksResident = 0;
    mUseSfxBank = 1;
    SelectSfxProgram();
}

// 0x003f4960
void Ps2HardSynth::LoadBankSet5() {
    mUseSfxBank = 1;

    HxStr bdName;
    QueryConfigString(&bdName, kBankSet5BdCode, GetHostMode());
    HxStr hdName;
    QueryConfigString(&hdName, kBankSet5HdCode, GetHostMode());
    LoadBankPair(bdName, hdName, kTagAllChannels, kPlacementFixedSecond);

    SelectSfxProgram();
    WaitForBankTransfers();
    AllNotesOff();
}

// 0x003f4af0
void Ps2HardSynth::LoadBankSet6() {
    mAlternateBanksResident = QueryConfigFlag(kAlternateBanksCode);

    HxStr bdName;
    QueryConfigString(&bdName, kBankSet6BdCode, mAlternateBanksResident, GetHostMode());
    HxStr hdName;
    QueryConfigString(&hdName, kBankSet6HdCode, mAlternateBanksResident, GetHostMode());
    LoadBankPair(
        bdName, hdName, kTagNone, mAlternateBanksResident ? kPlacementRotating : kPlacementBuffer);

    WaitForBankTransfers();
    ConfigureSpu2Effects(1);
    AllNotesOff();
}

// 0x003f6650
void Ps2HardSynth::UnloadBanks() {
    ReleaseSoundBanks();
}

// 0x003f66d8
void Ps2HardSynth::SendMidi(unsigned char nStatus, unsigned char nData1, unsigned char nData2) {
    SendMidiToDriver(nStatus, nData1, nData2);
}

// 0x003f65c8
void Ps2HardSynth::SelectBank(unsigned char nChannel, unsigned char nBank) {
    if (mAlternateBanksResident == 0) {
        return;
    }

    const unsigned char nStatus = kStatusControlChange | nChannel;
    SendMidi(nStatus, kControllerBankSelectMsb, 0);
    SendMidi(nStatus, kControllerBankSelectLsb, nBank);
}

// 0x003f6700
void Ps2HardSynth::Slot12(int bEnable) {
    SubmitDriverSelector110(bEnable ^ 1); // The driver command takes the opposite sense.
}

// 0x003f6740
void Ps2HardSynth::Slot13(int nValue) {
    SubmitDriverSelector100(nValue);
}

// 0x003f6720
void Ps2HardSynth::Slot14(int nValue) {
    SubmitDriverSelectorF0(nValue);
}

// 0x003f4c50
void Ps2HardSynth::AllNotesOff() {
    Synth::AllNotesOff();

    for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
        const unsigned char nStatus = kStatusControlChange | nChannel;
        SendMidi(nStatus, kControllerExpression, kFullExpression);
        SendMidi(nStatus, kControllerBankSelectMsb, 0);
        SendMidi(nStatus, kControllerBankSelectLsb, 0);
        SendMidi(kStatusProgramChange | nChannel, 0, 0);
    }

    SetChannelVolume(kDefaultVolume);
    SelectSfxProgram();
    SendMidi(kStatusProgramChange | kMuseChannel, kMuseProgram, 0);
}

// 0x003f4590
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

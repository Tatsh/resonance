#include "synth/synth.h"

#include "msg/stdmidimsg.h"

namespace {

constexpr int kChannelCount = 16;
constexpr int kSfxChannel = 15;
constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kControllerChannelVolume = 7;
constexpr unsigned char kControllerAllNotesOff = 123;

} // namespace

// 0x0013a398
Synth::~Synth() {
}

// 0x0013a1d0
void Synth::LoadBankSet4() {
}

// 0x0013a1d8
void Synth::LoadBankSet5() {
}

// 0x0013a1e0
void Synth::LoadBankSet6() {
}

// 0x0013a1e8
void Synth::Slot7() {
}

// 0x0013a1f0
void Synth::UnloadBanks() {
}

// 0x0013a1f8
void Synth::Slot10() {
}

// 0x003f65c8 is the one implementation. This body is empty.
void Synth::SelectBank([[maybe_unused]] unsigned char nChannel,
                       [[maybe_unused]] unsigned char nBank) {
}

// 0x0013a208
void Synth::Slot12([[maybe_unused]] int bEnable) {
}

// 0x0013a210
void Synth::Slot13([[maybe_unused]] int nValue) {
}

// 0x0013a218
void Synth::Slot14([[maybe_unused]] int nValue) {
}

// 0x0013a220
void Synth::AllNotesOff() {
    for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
        SendMidi(kStatusControlChange | nChannel, kControllerAllNotesOff, 0);
    }
}

// 0x0013a288
void Synth::AllNotesOffExceptSfxChannel() {
    for (unsigned char nChannel = 0; nChannel < kSfxChannel; ++nChannel) {
        SendMidi(kStatusControlChange | nChannel, kControllerAllNotesOff, 0);
    }
}

// 0x0013a2f0
void Synth::SetChannelVolume(unsigned char nVolume) {
    for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
        SendMidi(kStatusControlChange | nChannel, kControllerChannelVolume, nVolume);
    }
}

// 0x0013a570
void Synth::HandleMessage(Message *pMsg) {
    if (pMsg->Type() != static_cast<int>(g_dwStdMidiMsgType)) {
        return;
    }

    StdMidiMsg *pMidi = static_cast<StdMidiMsg *>(pMsg);
    SendMidi(pMidi->mUnknown08, pMidi->mUnknown09, pMidi->mUnknown0a);
}

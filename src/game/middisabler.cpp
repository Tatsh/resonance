#include "game/middisabler.h"

#include "msg/allnotesoffmsg.h"
#include "msg/message.h"
#include "msg/notemsg.h"
#include "msg/stdmidimsg.h"

namespace {

// The high nibble of a StdMidiMsg status, which selects the kind of message.
constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusNoteOff = 0x80;
constexpr unsigned char kStatusNoteOn = 0x90;

} // namespace

// NTSC-U/C: 0x001a6e10, PAL: 0x001acb78
MidiDisabler::MidiDisabler(int bEnabled) : mEnabled(bEnabled) {
}

// NTSC-U/C: 0x001a6ac0, PAL: 0x001ac828
MidiDisabler::~MidiDisabler() {
}

// NTSC-U/C: 0x001a6f08, PAL: 0x001acc70
void MidiDisabler::HandleMessage(Message *pMsg) {
    const unsigned int dwType = pMsg->Type();
    if (dwType == g_dwStdMidiMsgType) {
        OnMsg(*static_cast<StdMidiMsg *>(pMsg));
    } else if (dwType == g_dwNoteMsgType) {
        OnMsg(*static_cast<NoteMsg *>(pMsg));
    } else {
        Send(pMsg);
    }
}

// NTSC-U/C: 0x001a6ef8, PAL: 0x001acc60
void MidiDisabler::Enable() {
    mEnabled = 1;
}

// NTSC-U/C: 0x001a6a58, PAL: 0x001ac7c0
void MidiDisabler::Disable() {
    mEnabled = 0;
    AllNotesOffMsg message;
    Send(&message);
}

// NTSC-U/C: 0x001a6e60, PAL: 0x001acbc8
void MidiDisabler::OnMsg(StdMidiMsg &msg) {
    if (mEnabled == 0) {
        const unsigned char nKind = msg.mStatus & kStatusKindMask;
        if (nKind == kStatusNoteOff || nKind == kStatusNoteOn) {
            return;
        }
    }
    Send(&msg);
}

// NTSC-U/C: 0x001a6eb0, PAL: 0x001acc18
void MidiDisabler::OnMsg(NoteMsg &msg) {
    if (mEnabled != 0) {
        Send(&msg);
    }
}

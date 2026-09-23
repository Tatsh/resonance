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

// 0x001a6e10
MidiDisabler::MidiDisabler(int bEnabled) : mEnabled(bEnabled) {
}

// 0x001a6ac0
MidiDisabler::~MidiDisabler() {
}

// 0x001a6f08
void MidiDisabler::HandleMessage(Message *pMsg) {
    const unsigned int dwType = pMsg->Type();
    if (dwType == g_dwStdMidiMsgType) {
        PassStdMidi(static_cast<StdMidiMsg *>(pMsg));
    } else if (dwType == g_dwNoteMsgType) {
        PassNote(static_cast<NoteMsg *>(pMsg));
    } else {
        Send(pMsg);
    }
}

// 0x001a6ef8
void MidiDisabler::Enable() {
    mEnabled = 1;
}

// 0x001a6a58
void MidiDisabler::Disable() {
    mEnabled = 0;
    AllNotesOffMsg message;
    Send(&message);
}

// 0x001a6e60
void MidiDisabler::PassStdMidi(StdMidiMsg *pMsg) {
    if (mEnabled == 0) {
        const unsigned char nKind = pMsg->mUnknown08 & kStatusKindMask;
        if (nKind == kStatusNoteOff || nKind == kStatusNoteOn) {
            return;
        }
    }
    Send(pMsg);
}

// 0x001a6eb0
void MidiDisabler::PassNote(NoteMsg *pMsg) {
    if (mEnabled != 0) {
        Send(pMsg);
    }
}

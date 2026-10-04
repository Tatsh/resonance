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

MidiDisabler::MidiDisabler(int bEnabled) : mEnabled(bEnabled) {
}

MidiDisabler::~MidiDisabler() {
}

void MidiDisabler::DispatchPriv(Message *pMsg) {
    const unsigned int dwType = pMsg->Type();
    if (dwType == StdMidiMsg::sID) {
        OnMsg(*static_cast<StdMidiMsg *>(pMsg));
    } else if (dwType == NoteMsg::sID) {
        OnMsg(*static_cast<NoteMsg *>(pMsg));
    } else {
        Send(pMsg);
    }
}

void MidiDisabler::Enable() {
    mEnabled = 1;
}

void MidiDisabler::Disable() {
    mEnabled = 0;
    AllNotesOffMsg message;
    Send(&message);
}

void MidiDisabler::OnMsg(StdMidiMsg &msg) {
    if (mEnabled == 0) {
        const unsigned char nKind = msg.mStatus & kStatusKindMask;
        if (nKind == kStatusNoteOff || nKind == kStatusNoteOn) {
            return;
        }
    }
    Send(&msg);
}

void MidiDisabler::OnMsg(NoteMsg &msg) {
    if (mEnabled != 0) {
        Send(&msg);
    }
}

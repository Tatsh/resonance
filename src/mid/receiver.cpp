#include "mid/receiver.h"

// 0x001ea248. Everything in the body is compiler-generated, so nothing of the class's own appears
// here.
Mid::Receiver::~Receiver() {
}

// 0x001ea278
void Mid::Receiver::NewTrack(unsigned char) {
}

// 0x001ea280
void Mid::Receiver::NoteOn(int, unsigned char, unsigned char, unsigned char) {
}

// 0x001ea288
void Mid::Receiver::NoteOff(int, unsigned char, unsigned char) {
}

// 0x001ea290
void Mid::Receiver::Controller(int, unsigned char, unsigned char, unsigned char) {
}

// 0x001ea298
void Mid::Receiver::ProgramChange(int, unsigned char, unsigned char) {
}

// 0x001ea2a0
void Mid::Receiver::PitchBend(int, unsigned char, unsigned char, unsigned char) {
}

// 0x001ea2a8
void Mid::Receiver::AllDone(int, void *) {
}

// 0x001ea2b0
void Mid::Receiver::TextEvent(int, const char *, unsigned char) {
}

// 0x001ea2b8
void Mid::Receiver::EndTrack() {
}

// 0x001ea2c0
void Mid::Receiver::OnUnknownSlot11() {
}

// 0x001ea2c8
void Mid::Receiver::OnUnknownSlot12() {
}

// 0x001ea2d0
void Mid::Receiver::OnUnknownSlot13() {
}

// 0x001ea2d8
void Mid::Receiver::OnUnknownSlot14() {
}

// 0x001ea2e0
void Mid::Receiver::OnUnknownSlot15() {
}

#include "mid/receiver.h"

// NTSC-U/C: 0x001ea248, PAL: 0x001f04b8
// Everything in the body is compiler-generated, so nothing of the class's own appears
// here.
Mid::Receiver::~Receiver() {
}

// NTSC-U/C: 0x001ea278, PAL: 0x001f04e8
void Mid::Receiver::NewTrack(unsigned char) {
}

// NTSC-U/C: 0x001ea280, PAL: 0x001f04f0
void Mid::Receiver::NoteOn(int, unsigned char, unsigned char, unsigned char) {
}

// NTSC-U/C: 0x001ea288, PAL: 0x001f04f8
void Mid::Receiver::NoteOff(int, unsigned char, unsigned char) {
}

// NTSC-U/C: 0x001ea290, PAL: 0x001f0500
void Mid::Receiver::Controller(int, unsigned char, unsigned char, unsigned char) {
}

// NTSC-U/C: 0x001ea298, PAL: 0x001f0508
void Mid::Receiver::ProgramChange(int, unsigned char, unsigned char) {
}

// NTSC-U/C: 0x001ea2a0, PAL: 0x001f0510
void Mid::Receiver::PitchBend(int, unsigned char, unsigned char, unsigned char) {
}

// NTSC-U/C: 0x001ea2a8, PAL: 0x001f0518
void Mid::Receiver::Tempo(int, int) {
}

// NTSC-U/C: 0x001ea2b0, PAL: 0x001f0520
void Mid::Receiver::TextEvent(int, const char *, unsigned char) {
}

// NTSC-U/C: 0x001ea2b8, PAL: 0x001f0528
void Mid::Receiver::EndTrack() {
}

// NTSC-U/C: 0x001ea2c0, PAL: 0x001f0530
void Mid::Receiver::AllDone() {
}

// NTSC-U/C: 0x001ea2c8, PAL: 0x001f0538
void Mid::Receiver::UnusedFirstHook() {
}

// NTSC-U/C: 0x001ea2d0, PAL: 0x001f0540
void Mid::Receiver::UnusedSecondHook() {
}

// NTSC-U/C: 0x001ea2d8, PAL: 0x001f0548
void Mid::Receiver::UnusedThirdHook() {
}

// NTSC-U/C: 0x001ea2e0, PAL: 0x001f0550
void Mid::Receiver::UnusedFourthHook() {
}

#include "mid/receiver.h"

Mid::Receiver::~Receiver() {
    // Everything in the body is compiler-generated, so nothing of the class's own appears here.
}

void Mid::Receiver::NewTrack(unsigned char) {
}

void Mid::Receiver::NoteOn(int, unsigned char, unsigned char, unsigned char) {
}

void Mid::Receiver::NoteOff(int, unsigned char, unsigned char) {
}

void Mid::Receiver::Controller(int, unsigned char, unsigned char, unsigned char) {
}

void Mid::Receiver::ProgramChange(int, unsigned char, unsigned char) {
}

void Mid::Receiver::PitchBend(int, unsigned char, unsigned char, unsigned char) {
}

void Mid::Receiver::Tempo(int, int) {
}

void Mid::Receiver::TextEvent(int, const char *, unsigned char) {
}

void Mid::Receiver::EndTrack() {
}

void Mid::Receiver::AllDone() {
}

void Mid::Receiver::UnusedFirstHook() {
}

void Mid::Receiver::UnusedSecondHook() {
}

void Mid::Receiver::UnusedThirdHook() {
}

void Mid::Receiver::UnusedFourthHook() {
}

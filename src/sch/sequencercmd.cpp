#include "sch/sequencercmd.h"

#include "sch/genericsequencer.h"

int SequencerCmd::sCmdID;

// NTSC-U/C: 0x00100b50, PAL: 0x00100b50
int SequencerCmd::CmdID() {
    return sCmdID;
}

// NTSC-U/C: 0x00100b60, PAL: 0x00100b60
void SequencerCmd::Execute() {
    mOwner->Dispatch();
}

// NTSC-U/C: 0x00100b90, PAL: 0x00100b90
void SequencerCmd::Print(std::ostream &stream) {
    stream << "{Sequencer}";
}

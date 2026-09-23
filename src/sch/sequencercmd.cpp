#include "sch/sequencercmd.h"

#include "sch/genericsequencer.h"

int SequencerCmd::sCmdID;

// 0x00100b50
int SequencerCmd::CmdID() {
    return sCmdID;
}

// 0x00100b60
void SequencerCmd::Execute() {
    mOwner->Dispatch();
}

// 0x00100b90
void SequencerCmd::Print(std::ostream &stream) {
    stream << "{Sequencer}";
}

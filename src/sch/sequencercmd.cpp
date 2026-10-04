#include "sch/sequencercmd.h"

#include "sch/genericsequencer.h"

int SequencerCmd::sCmdID;

int SequencerCmd::CmdID() {
    return sCmdID;
}

void SequencerCmd::Execute() {
    mOwner->Dispatch();
}

void SequencerCmd::Print(std::ostream &stream) {
    stream << "{Sequencer}";
}

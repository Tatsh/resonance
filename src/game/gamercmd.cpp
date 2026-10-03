#include "game/gamercmd.h"

#include <iostream>

namespace {

constexpr char kDescription[] = "{Gamer}";

} // namespace

// The destructor slot of the table at `0x007ce600` holds the inherited `Sch::Command`
// destructor at `0x00116b48`, which installs the base table at `0x008288c0`; this class
// declares no destructor of its own.

// NTSC-U/C: 0x00116bc0, PAL: 0x00117078
int GamerCmd::CmdID() {
    return g_nGamerCmdID;
}

// NTSC-U/C: 0x00116bd0, PAL: 0x00117088
void GamerCmd::Execute() {
    mGamer->OnBar(mBar);
}

// NTSC-U/C: 0x00116bf0, PAL: 0x001170a8
void GamerCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

int g_nGamerCmdID;

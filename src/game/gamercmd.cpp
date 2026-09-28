#include "game/gamercmd.h"

#include <iostream>

namespace {

constexpr char kDescription[] = "{Gamer}";

} // namespace

// The destructor slot of the table at `0x007ce600` holds the inherited `Sch::Command`
// destructor at `0x00116b48`, which installs the base table at `0x008288c0`; this class
// declares no destructor of its own.

// 0x00116bc0
int GamerCmd::CmdID() {
    return g_nGamerCmdID;
}

// 0x00116bd0
void GamerCmd::Execute() {
    mGamer->OnBar(mBar);
}

// 0x00116bf0
void GamerCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

int g_nGamerCmdID;

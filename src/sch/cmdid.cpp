#include "sch/cmdid.h"

#include <iostream>
#include <set>

#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// The next handle value to hand out, which starts at one so that no handle is zero.
// NTSC-U/C: 0x0077d2e8, PAL: 0x007c10c0
int g_nNextCmdIdValue = 1;

// Handle values a replayed recording has reserved.
// NTSC-U/C: 0x008e4f08, PAL: 0x00929f08
std::set<int> sReserved;

// NTSC-U/C: 0x008e4f18, PAL: 0x00929f18
// The first reserved value the counter has not yet passed.
std::set<int>::iterator sIter = sReserved.end();

} // namespace

// NTSC-U/C: 0x005e4dc8, PAL: 0x00626f88
int CmdID::AllocateValue() {
    while (sIter != sReserved.end()) {
        if (*sIter != g_nNextCmdIdValue) {
            break;
        }
        ++sIter;
        ++g_nNextCmdIdValue;
    }
    return g_nNextCmdIdValue++;
}

// NTSC-U/C: 0x005e5908, PAL: 0x00627ac8
void CmdID::ReserveID(CmdID id) {
    sReserved.insert(id.mValue);
    sIter = sReserved.begin();
}

// NTSC-U/C: 0x005e59a0, PAL: 0x00627b60
OBStream &CmdID::Save(OBStream &stream) {
    int nValue = mValue;
    return stream.Write(&nValue, sizeof(nValue));
}

// NTSC-U/C: 0x005e59e0, PAL: 0x00627ba0
IBStream &CmdID::Load(IBStream &stream) {
    return stream.Read(&mValue, sizeof(mValue));
}

// NTSC-U/C: 0x005e5958, PAL: 0x00627b18
void CmdID::Print(std::ostream &stream) {
    stream << "{cmdID " << mValue << '}';
}

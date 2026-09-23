#include "sch/cmdid.h"

#include <iostream>
#include <set>

#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// 0x0077d2e8. The next handle value to hand out, which starts at one so that no handle is zero.
int g_nNextCmdIdValue = 1;

// 0x008e4f08. Handle values a replayed recording has reserved.
std::set<int> g_reservedCmdIdValues;

// 0x008e4f18
// The first reserved value the counter has not yet passed.
std::set<int>::iterator g_itNextReservedCmdIdValue = g_reservedCmdIdValues.end();

} // namespace

// 0x005e4dc8
int CmdID::AllocateValue() {
    while (g_itNextReservedCmdIdValue != g_reservedCmdIdValues.end()) {
        if (*g_itNextReservedCmdIdValue != g_nNextCmdIdValue) {
            break;
        }
        ++g_itNextReservedCmdIdValue;
        ++g_nNextCmdIdValue;
    }
    return g_nNextCmdIdValue++;
}

// 0x005e5908
void CmdID::Reserve(CmdID id) {
    g_reservedCmdIdValues.insert(id.mValue);
    g_itNextReservedCmdIdValue = g_reservedCmdIdValues.begin();
}

// 0x005e59a0
OBStream &CmdID::Save(OBStream &stream) {
    int nValue = mValue;
    return stream.Write(&nValue, sizeof(nValue));
}

// 0x005e59e0
IBStream &CmdID::Load(IBStream &stream) {
    return stream.Read(&mValue, sizeof(mValue));
}

// 0x005e5958
void CmdID::Print(std::ostream &stream) {
    stream << "{cmdID " << mValue << '}';
}

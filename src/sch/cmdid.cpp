#include "sch/cmdid.h"

#include "stream/ibstream.h"
#include "stream/obstream.h"

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
void CmdID::Print(ostream &stream) {
    stream << "{cmdID " << mValue << '}';
}

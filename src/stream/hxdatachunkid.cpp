#include "stream/hxdatachunkid.h"

#include <iostream>

#include "stream/hxstream.h"

// 0x00145fe0
HxStream &HxDataChunkId::Read(HxStream &stream) {
    (stream >> mName).ReadSwapped(&mSize, sizeof(mSize));
    if (mName == g_listChunkName || mName == g_riffChunkName) {
        stream >> mName;
        mIsList = 1;
        mSize -= HxChunkName::kLength;
    } else {
        mIsList = 0;
    }
    return stream;
}

// 0x00145868
void HxDataChunkId::Print(std::ostream &stream) {
    std::ostream &rest = mIsList != 0 ? stream << "LIST:" : stream;
    rest << mName.mText[0] << mName.mText[1] << mName.mText[2] << mName.mText[3] << "<" << mSize
         << ">";
}

// 0x001464b8
HxStream &operator>>(HxStream &stream, HxDataChunkId &id) {
    return id.Read(stream); // The binary expands Read() inline here.
}

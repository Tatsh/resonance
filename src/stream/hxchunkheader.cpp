#include "stream/hxchunkheader.h"

#include <iostream>

#include "stream/hxstream.h"

// NTSC-U/C: 0x00145fe0, PAL: 0x00146af8
HxStream &HxChunkHeader::Read(HxStream &stream) {
    (stream >> mName).ReadNum(&mSize, sizeof(mSize));
    if (mName == kListChunkID || mName == kRiffChunkID) {
        stream >> mName;
        mIsList = 1;
        mSize -= HxChunkName::kLength;
    } else {
        mIsList = 0;
    }
    return stream;
}

// NTSC-U/C: 0x00145868, PAL: 0x00146380
void HxChunkHeader::Print(std::ostream &stream) {
    std::ostream &rest = mIsList != 0 ? stream << "LIST:" : stream;
    rest << mName.mText[0] << mName.mText[1] << mName.mText[2] << mName.mText[3] << "<" << mSize
         << ">";
}

// NTSC-U/C: 0x001464b8, PAL: 0x00146fd0
HxStream &operator>>(HxStream &stream, HxChunkHeader &id) {
    return id.Read(stream); // The binary expands Read() inline here.
}

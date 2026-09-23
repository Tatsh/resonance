#include "msg/transportaddress.h"

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003f43a8
TransportAddress::TransportAddress(const HxStr &unknown00,
                                   const HxStr &unknown08,
                                   const HxStr &unknown10,
                                   int nUnknown18)
    : mUnknown00(unknown00), mUnknown08(unknown08), mUnknown10(unknown10), mUnknown18(nUnknown18) {
}

// 0x003f4478. The body is the three member destructors, which the compiler expands.
TransportAddress::~TransportAddress() {
}

// 0x003f4078
void TransportAddress::Save(OBStream &stream) {
    int nUnknown18 = mUnknown18; // The binary writes a stack copy of the word.
    SaveHxStr(SaveHxStr(SaveHxStr(stream, mUnknown00), mUnknown08), mUnknown10)
        .Write(&nUnknown18, sizeof(nUnknown18));
}

// 0x003f41d0
void TransportAddress::Load(IBStream &stream) {
    LoadHxStr(LoadHxStr(LoadHxStr(stream, mUnknown00), mUnknown08), mUnknown10)
        .Read(&mUnknown18, sizeof(mUnknown18));
}

// 0x003f4500
HxStr TransportAddress::GetUnknown00() {
    return mUnknown00;
}

// 0x003f4528
HxStr TransportAddress::GetUnknown08() {
    return mUnknown08;
}

// 0x003f4558
HxStr TransportAddress::GetUnknown10() {
    return mUnknown10;
}

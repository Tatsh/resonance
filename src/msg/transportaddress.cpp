#include "msg/transportaddress.h"

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003f43a8, PAL: 0x0042c990
TransportAddress::TransportAddress(const HxStr &host,
                                   const HxStr &address,
                                   const HxStr &service,
                                   int nPort)
    : mHost(host), mAddress(address), mService(service), mPort(nPort) {
}

// NTSC-U/C: 0x003f4478, PAL: 0x0042ca80
// The body is the three member destructors, which the compiler expands.
TransportAddress::~TransportAddress() {
}

// NTSC-U/C: 0x003f4078, PAL: 0x0042c658
void TransportAddress::Save(OBStream &stream) {
    int nPort = mPort; // The binary writes a stack copy of the word.
    SaveHxStr(SaveHxStr(SaveHxStr(stream, mHost), mAddress), mService)
        .WriteLE(&nPort, sizeof(nPort));
}

// NTSC-U/C: 0x003f41d0, PAL: 0x0042c7b0
void TransportAddress::Load(IBStream &stream) {
    LoadHxStr(LoadHxStr(LoadHxStr(stream, mHost), mAddress), mService)
        .ReadLE(&mPort, sizeof(mPort));
}

// NTSC-U/C: 0x003f4500, PAL: 0x0042cb38
HxStr TransportAddress::GetHost() {
    return mHost;
}

// NTSC-U/C: 0x003f4528, PAL: 0x0042cb60
HxStr TransportAddress::GetAddress() {
    return mAddress;
}

// NTSC-U/C: 0x003f4558, PAL: 0x0042cb90
HxStr TransportAddress::GetService() {
    return mService;
}

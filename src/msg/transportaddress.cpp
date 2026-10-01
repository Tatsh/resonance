#include "msg/transportaddress.h"

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003f43a8
TransportAddress::TransportAddress(const HxStr &host,
                                   const HxStr &address,
                                   const HxStr &service,
                                   int nPort)
    : mHost(host), mAddress(address), mService(service), mPort(nPort) {
}

// 0x003f4478
// The body is the three member destructors, which the compiler expands.
TransportAddress::~TransportAddress() {
}

// 0x003f4078
void TransportAddress::Save(OBStream &stream) {
    int nPort = mPort; // The binary writes a stack copy of the word.
    SaveHxStr(SaveHxStr(SaveHxStr(stream, mHost), mAddress), mService).Write(&nPort, sizeof(nPort));
}

// 0x003f41d0
void TransportAddress::Load(IBStream &stream) {
    LoadHxStr(LoadHxStr(LoadHxStr(stream, mHost), mAddress), mService).Read(&mPort, sizeof(mPort));
}

// 0x003f4500
HxStr TransportAddress::GetHost() {
    return mHost;
}

// 0x003f4528
HxStr TransportAddress::GetAddress() {
    return mAddress;
}

// 0x003f4558
HxStr TransportAddress::GetService() {
    return mService;
}

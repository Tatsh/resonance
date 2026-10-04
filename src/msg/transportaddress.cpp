#include "msg/transportaddress.h"

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

TransportAddress::TransportAddress(const HxStr &host,
                                   const HxStr &address,
                                   const HxStr &service,
                                   int nPort)
    : mHost(host), mAddress(address), mService(service), mPort(nPort) {
}

TransportAddress::~TransportAddress() {
    // The body is the three member destructors the compiler expands.
}

void TransportAddress::Save(OBStream &stream) {
    int nPort = mPort; // The binary writes a stack copy of the word.
    SaveHxStr(SaveHxStr(SaveHxStr(stream, mHost), mAddress), mService)
        .WriteLE(&nPort, sizeof(nPort));
}

void TransportAddress::Load(IBStream &stream) {
    LoadHxStr(LoadHxStr(LoadHxStr(stream, mHost), mAddress), mService)
        .ReadLE(&mPort, sizeof(mPort));
}

HxStr TransportAddress::GetHost() {
    return mHost;
}

HxStr TransportAddress::GetAddress() {
    return mAddress;
}

HxStr TransportAddress::GetService() {
    return mService;
}

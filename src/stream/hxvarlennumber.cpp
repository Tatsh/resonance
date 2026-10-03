#include "stream/hxvarlennumber.h"

#include "stream/hxstream.h"

namespace {

constexpr int kVarLenBits = 7;
constexpr unsigned char kVarLenValueMask = 0x7f;
constexpr unsigned char kVarLenContinue = 0x80;
constexpr int kBitsPerByte = 8;

} // namespace

// NTSC-U/C: 0x00405ad8, PAL: 0x0043f3c8
HxStream &HxVarLenNumber::Write(HxStream &stream) const {
    int nRemaining = mValue;
    int nPacked = nRemaining & kVarLenValueMask;
    int nBytes = 1;
    while ((nRemaining >>= kVarLenBits) != 0) {
        nPacked = (nPacked << kBitsPerByte) | kVarLenContinue | (nRemaining & kVarLenValueMask);
        ++nBytes;
    }
    for (; nBytes != 0; --nBytes) {
        const unsigned char byte = static_cast<unsigned char>(nPacked);
        stream.WriteNum(&byte, sizeof(byte));
        nPacked >>= kBitsPerByte;
    }
    return stream;
}

// NTSC-U/C: 0x00405b70, PAL: 0x0043f460
HxStream &HxVarLenNumber::Read(HxStream &stream) {
    mValue = 0;
    unsigned char byte;
    do {
        stream.ReadNum(&byte, sizeof(byte));
        mValue = (mValue << kVarLenBits) + (byte & kVarLenValueMask);
    } while ((byte & kVarLenContinue) != 0);
    return stream;
}

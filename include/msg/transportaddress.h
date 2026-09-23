#pragma once

#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * Network address a packet transport serialises: three strings and one word.
 *
 * `16TransportAddress` in the RTTI descriptor at `0x0086f6e8`, with no base. The three strings
 * come first, then the word, then the vptr at `+0x1c` (the position of a vptr in a class with no
 * base). Its table at `0x00814e10` has four entries (the type function at `0x003f4330`, the
 * destructor, Save(), and Load()).
 *
 * No routine of the class has a caller in the image, and no string names a field. The member
 * names are placeholders that record their offsets.
 */
class TransportAddress {
public:
    /**
     * Construct from copies of the three strings and the word.
     *
     * @param unknown00 The first string.
     * @param unknown08 The second string.
     * @param unknown10 The third string.
     * @param nUnknown18 The word.
     * @ghidraAddress 0x003f43a8
     */
    TransportAddress(const HxStr &unknown00,
                     const HxStr &unknown08,
                     const HxStr &unknown10,
                     int nUnknown18);

    /**
     * Slot 1. The body frees the three string buffers, in reverse order, and nothing else.
     *
     * @ghidraAddress 0x003f4478
     */
    virtual ~TransportAddress();

    /**
     * Write the three strings through SaveHxStr() and then the word, as four bytes.
     *
     * Slot 2. Each write goes to the stream the previous write returned.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f4078
     */
    virtual void Save(OBStream &stream);

    /**
     * Read back what Save() writes, through LoadHxStr() and a four-byte read.
     *
     * Slot 3.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003f41d0
     */
    virtual void Load(IBStream &stream);

    /**
     * Report a copy of the first string.
     *
     * @return The first string.
     * @ghidraAddress 0x003f4500
     */
    HxStr GetUnknown00();

    /**
     * Report a copy of the second string.
     *
     * @return The second string.
     * @ghidraAddress 0x003f4528
     */
    HxStr GetUnknown08();

    /**
     * Report a copy of the third string.
     *
     * @return The third string.
     * @ghidraAddress 0x003f4558
     */
    HxStr GetUnknown10();

private:
    HxStr mUnknown00; // +0x00
    HxStr mUnknown08; // +0x08
    HxStr mUnknown10; // +0x10
    int mUnknown18;   // +0x18
};

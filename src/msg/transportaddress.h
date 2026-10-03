#pragma once

#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * Network address a packet transport serialises: three strings and one word.
 *
 * Its RTTI descriptor is at `0x0086f6e8`. It has no base. The three strings come first, then the
 * word, then the vptr at `+0x1c` (the position of a vptr in a class with no base). Its table at
 * `0x00814e10` has four entries (the type function at `0x003f4330`, the destructor, Save(), and
 * Load()).
 *
 * No routine of the class has a caller in the image, and no string names a field. The member
 * titles are inferred from the class name and from the options labels `Net IP Address` and
 * `Net Port` (an address paired with a port number).
 */
class TransportAddress {
public:
    /**
     * Construct from copies of the three strings and the word.
     *
     * @param host The first string.
     * @param address The second string.
     * @param service The third string.
     * @param nPort The word.
     * @ghidraAddress NTSC-U/C: 0x003f43a8
     * @ghidraAddress PAL: 0x0042c990
     */
    TransportAddress(const HxStr &host, const HxStr &address, const HxStr &service, int nPort);

    /**
     * Slot 1. The body frees the three string buffers, in reverse order, and nothing else.
     *
     * @ghidraAddress NTSC-U/C: 0x003f4478
     * @ghidraAddress PAL: 0x0042ca80
     */
    virtual ~TransportAddress();

    /**
     * Write the three strings through SaveHxStr() and then the word, as four bytes.
     *
     * Slot 2. Each write goes to the stream the previous write returned.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f4078
     * @ghidraAddress PAL: 0x0042c658
     */
    virtual void Save(OBStream &stream);

    /**
     * Read back what Save() writes, through LoadHxStr() and a four-byte read.
     *
     * Slot 3.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003f41d0
     * @ghidraAddress PAL: 0x0042c7b0
     */
    virtual void Load(IBStream &stream);

    /**
     * Report a copy of the first string.
     *
     * @return The first string.
     * @ghidraAddress NTSC-U/C: 0x003f4500
     * @ghidraAddress PAL: 0x0042cb38
     */
    HxStr GetHost();

    /**
     * Report a copy of the second string.
     *
     * @return The second string.
     * @ghidraAddress NTSC-U/C: 0x003f4528
     * @ghidraAddress PAL: 0x0042cb60
     */
    HxStr GetAddress();

    /**
     * Report a copy of the third string.
     *
     * @return The third string.
     * @ghidraAddress NTSC-U/C: 0x003f4558
     * @ghidraAddress PAL: 0x0042cb90
     */
    HxStr GetService();

private:
    HxStr mHost;    // +0x00
    HxStr mAddress; // +0x08
    HxStr mService; // +0x10
    int mPort;      // +0x18
};

#pragma once

#include <iostream>

#include "game/gameparams.h"
#include "msg/transportaddress.h"
#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * One advertised network game: where it is hosted and the settings it plays.
 *
 * `8GameInfo` in the RTTI descriptor built by the type function at `0x001879c8`, with no base. The
 * members are a TransportAddress at `+0x00`, a word at `+0x20`, a string at `+0x24`, a GameParams
 * at `+0x2c`, and a word at `+0x64`, and the vptr follows at `+0x68`. Its table at `0x007db578`
 * has four entries (the type function, the destructor, Save(), and Load()). The member titles
 * after the address come from the labels Print() writes, `mediusWorldID=`, ` host=`, and
 * ` status=`.
 *
 * No constructor is emitted, and no routine of the class has a caller in the image.
 */
class GameInfo {
public:
    /**
     * Slot 1. The body is empty, and the compiler frees the members' strings in reverse order.
     *
     * @ghidraAddress 0x00187a08
     */
    virtual ~GameInfo();

    /**
     * Write the address, the world identifier, the host, the settings, and the status.
     *
     * Everything after the address is chained through the stream each transfer returns.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001876f8
     */
    virtual void Save(OBStream &stream);

    /**
     * Read back what Save() writes.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00187800
     */
    virtual void Load(IBStream &stream);

    /**
     * Write the fields to a diagnostic stream.
     *
     * The label `addr=` is written with nothing after it, and `mediusWorldID=` follows at once, so
     * the address itself is not printed.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00187c60
     */
    void Print(std::ostream &stream);

private:
    TransportAddress mAddress;
    unsigned int mMediusWorldId;
    HxStr mHost;
    GameParams mParams;
    int mStatus;
};

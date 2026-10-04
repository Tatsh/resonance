#pragma once

#include <iostream>
#include <vector>

#include "os/hxstr.h"

/**
 * Registry that routes each source file's diagnostic stream to a named output channel.
 *
 * The class emits no RTTI. Its name comes from the debugging symbols of the North American demo
 * release. The one instance is a function-local static of shared() at `0x00894df8`, 0x18 bytes,
 * destroyed at exit through the thunk at `0x004b4660`.
 *
 * A source file registers the address of its stream pointer under its own base name through
 * SpewRegister. Connect() then points every registered stream of a file at a channel, creating the
 * channel on first use. A channel is the console, nothing, or a file of that name.
 */
class SpewTable {
public:
    /** Output channel, 0xc bytes, created by Connect() and destroyed by the destructor. */
    struct Channel {
        /**
         * Construct a channel.
         *
         * @param name The channel name.
         * @param pStream The stream, or null for a channel that discards its output.
         */
        Channel(const HxStr &name, std::ostream *pStream) : mName(name), mStream(pStream) {
        }

        HxStr mName;           /*!< The lower-case channel name. */
        std::ostream *mStream; /*!< The stream, or null. */
    };

    /** One source file's registration, 0x10 bytes. */
    struct Connection {
        /**
         * Construct a registration.
         *
         * @param file The lower-case base name of the source file.
         * @param ppStream The file's stream pointer.
         * @param pChannel The channel, or null before Connect() reaches the file.
         */
        Connection(const HxStr &file, std::ostream **ppStream, Channel *pChannel)
            : mFile(file), mppStream(ppStream), mpChannel(pChannel) {
        }

        HxStr mFile;              /*!< The lower-case base name of the source file. */
        std::ostream **mppStream; /*!< The file's stream pointer, which Connect() writes. */
        Channel *mpChannel;       /*!< The channel, or null. */
    };

    /**
     * Construct an empty registry.
     *
     * @ghidraAddress NTSC-U/C: 0x004b32b8
     * @ghidraAddress PAL: 0x004f14f8
     */
    SpewTable();

    /**
     * Close every channel and release both lists.
     *
     * @ghidraAddress NTSC-U/C: 0x004b32e8
     * @ghidraAddress PAL: 0x004f1528
     */
    ~SpewTable();

    /**
     * Report the one registry, constructing it on first use.
     *
     * The body is inline. SpewRegister's constructor expands it, and the script binding calls the
     * out-of-line copy.
     *
     * @return The registry.
     * @ghidraAddress NTSC-U/C: 0x004b4470
     * @ghidraAddress PAL: 0x004f2780
     */
    static SpewTable &shared() {
        static SpewTable instance;
        return instance;
    }

    /**
     * Register a source file's stream pointer.
     *
     * The file name loses its extension and its directory, is lower-cased, and is appended with no
     * channel.
     *
     * @param ppStream The file's stream pointer.
     * @param pszFile The source file name.
     * @ghidraAddress NTSC-U/C: 0x004b3528
     * @ghidraAddress PAL: 0x004f1790
     */
    void Register(std::ostream **ppStream, const char *pszFile);

    /**
     * Point the registered streams of a source file at a channel.
     *
     * The channel name is lower-cased and the channel created on first use. The file name is
     * lower-cased, and the first registration of that name receives the channel's stream. A file
     * that was never registered only creates the channel.
     *
     * @param file The source file's base name.
     * @param channel The channel name: `off`, `console`, `debug`, or a file to write.
     * @ghidraAddress NTSC-U/C: 0x004b36b8
     * @ghidraAddress PAL: 0x004f1960
     */
    void Connect(const HxStr &file, const HxStr &channel);

    /**
     * Write every registration and its channel name, one per line.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x004b4550
     * @ghidraAddress PAL: 0x004f2860
     */
    void PrintConnections(std::ostream &stream);

private:
    /**
     * Builds a channel.
     *
     * `off` and `debug` discard their output, `console` writes to cout, and any other name opens a
     * file for writing. The receiver is unread.
     *
     * @ghidraAddress NTSC-U/C: 0x004b3898
     * @ghidraAddress PAL: 0x004f1b80
     */
    Channel *NewChannel(const HxStr &name);

    /**
     * Flushes and deletes every channel's stream, deletes the channels, and empties mChannels.
     *
     * The destructor is the one caller.
     *
     * @ghidraAddress NTSC-U/C: 0x004b3448
     * @ghidraAddress PAL: 0x004f16a0
     */
    void CloseChannels();

    std::vector<Channel *> mChannels;     // +0x00
    std::vector<Connection> mConnections; // +0x0c
};

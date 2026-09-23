#pragma once

#include <iostream>

#include "os/mem.h"

class IBStream;
class OBStream;

/**
 * Base of every event the game passes between a MsgSource and a MsgSink.
 *
 * `7Message` in the RTTI descriptor at `0x0086f638`, with no base and no data members, so the
 * compiler-generated vptr lands at offset 0 and every derived message adds its payload after it.
 * The RTTI lists 124 derived classes, among them ScriptMsg, GemMsg, LeaveGameMsg, and Packet.
 *
 * Every vtable in the family is eight entries, and the declaration order below reproduces the
 * order after the compiler-generated slot 0. Each one was read off the concrete implementations
 * rather than off the declaration: a scan of the derived tables found 80 classes that implement
 * Clone(), Type(), and Name(), while Packet and eight others inherit all three as pure and are
 * therefore abstract themselves.
 *
 * This class's own table is at `0x007ccbf8`, which its destructor stores at offset 0. It has eight
 * entries and a zero terminator at index 8, with slots 2, 3, and 4 addressing the pure-virtual
 * stub. MsgSink's table follows it immediately at `0x007ccc40`.
 *
 * Slots 5, 6, and 7 all address a two-instruction `jr ra` stub in this class, at `0x001051f0`,
 * `0x001051f8`, and `0x00105200`. Every translation unit that destroys a message emits its own
 * copy of all three stubs after its copy of the destructor, which is a second measurement of the
 * count.
 *
 * The three pure virtuals follow one pattern per class. LeaveGameMsg at `0x00812078` and GemMsg at
 * `0x00812588` are the two worked examples cited below.
 *
 * A concrete message also supplies a static New() that returns a default-constructed instance on
 * the heap, and the translation unit at `0x003d9818` registers 92 of them against the identity
 * Type() reports. Each registration constructs one file-scope object of a 4-byte class with the
 * identity word immediately below it, passing the identity and the factory to the MessageFactory
 * constructor, which inserts the pair into a sorted vector and stores nothing in the object.
 */
class Message {
public:
    /**
     * Allocate a message from the tagged heap under the tag `MSG`.
     *
     * Defined in the class, because 69 byte-identical copies exist, one in every translation unit
     * that allocates a message, and no call to any of them remains outside those units. The address
     * below is the first copy, and the program marks the rest as copies of it.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x003da1c0
     */
    void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "MSG");
    }

    /**
     * Release a message to the tagged heap under the tag `MSG`.
     *
     * Defined in the class for the same reason as operator new(), with 69 copies.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x003da1e0
     */
    void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "MSG");
    }

    /**
     * Vtable slot 1. The body is empty.
     *
     * @ghidraAddress 0x001051c0
     */
    virtual ~Message();

    /**
     * Produce a heap copy of this message.
     *
     * An implementation allocates the derived size against the tag `MSG`, installs its own vtable,
     * and copies its payload word by word. GemMsg's copies 0x18 bytes and LeaveGameMsg's copies
     * only the vptr, because that class has no payload. MsgQueue stores the result and its
     * destructor deletes every stored message, so the queue owns the copies.
     *
     * @return The copy.
     */
    virtual Message *Clone() = 0;

    /**
     * Report this message's registered identity.
     *
     * An implementation returns one word from the per-class identity table that spans `0x006d01ac`
     * through `0x006d035c`. A sink compares the result against the identity it listens for, which
     * is what ScriptSink::HandleMessage() does against `0x006d024c`. The read has no side effect,
     * so the call MsgQueue::HandleMessage() makes and discards has none either.
     *
     * @return The identity.
     */
    virtual int Type() = 0;

    /**
     * Report this message's class name.
     *
     * An implementation returns its own name as a string literal. LeaveGameMsg returns
     * `LeaveGameMsg` from `0x008116a8` and GemMsg returns `GemMsg` from `0x00811570`.
     *
     * @return The name.
     */
    virtual const char *Name() = 0;

    /**
     * Write this message's payload to a stream.
     *
     * The default writes nothing. GemMsg's override at `0x003d8830` streams its fields, so the
     * member exists for diagnostics.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001051f0
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write this message's payload to an output stream.
     *
     * Vtable slot 6. The default does nothing. An override copies each field into a temporary and
     * hands the temporary to OBStream::Write(), chaining on the stream the call returns, which is
     * how PSJoinRequestPacket's override at `0x003e5538` writes its four words and then delegates
     * its FreqAppearance member to that class's own slot 2. The verb is inferred from the shape
     * and from the position ahead of the reading half, since nothing in the image titles either
     * slot.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001051f8
     */
    virtual void Save(OBStream &stream);

    /**
     * Read this message's payload from an input stream.
     *
     * Vtable slot 7. The default does nothing. An override passes the address of each field
     * straight to IBStream::Read() so that the transfer fills the field in place, which is how
     * PSJoinRequestPacket's override at `0x003e5638` reads its four words and then delegates its
     * FreqAppearance member to that class's own slot 3. Twenty-two of the classes that override
     * this pair are packets and four are MIDI messages, so the pair is the wire format rather
     * than a diagnostic.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00105200
     */
    virtual void Load(IBStream &stream);

    /**
     * Write this message to a diagnostic stream as `{Name() Print()}`.
     *
     * The payload comes from Print(). PrintMuseEntry() is the one caller, and it discards the
     * result. The title is inferred.
     *
     * @param stream The stream to write to.
     * @return The stream.
     * @ghidraAddress 0x00556290
     */
    std::ostream &PrintBraced(std::ostream &stream);

    /**
     * Produce a message of the identified class through the factory list.
     *
     * The list is searched with a binary search on the identity, and a hit calls the factory a
     * MessageFactory registered. The shipped program does not call it, and
     * ReadMessagePointerFromStream() expands the same search inline.
     *
     * @param nType The identity to construct for.
     * @return The new message, or null when the identity is unregistered.
     * @ghidraAddress 0x005563d0
     */
    static Message *NewMessage(int nType);
};

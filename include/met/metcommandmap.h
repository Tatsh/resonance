#pragma once

#include "met/metscreen.h"

/**
 * One controller reading, as RawControllerMsg carries it.
 *
 * The four fields are recovered rather than inferred. RawControllerMsg::Clone() at `0x003da200`
 * copies them, RawController's one virtual at `0x005381a8` receives them as three ints followed by
 * a float in this order, and MetRenderer::HandleMessage() passes the address of the first of them
 * to MetCommandMap::Translate(). The record sits at `+0x04` of the message, so the three words
 * MetRenderer reads at message offsets 4, 8, and 0x0c are mTag, mPadIndex, and mButton.
 *
 * The name is inferred. The record belongs with RawControllerMsg rather than here, and it is
 * declared here because msg/rawcontrollermsg.h does not declare the payload yet.
 */
struct MetControllerReading {
    int mTag;      /*!< Four characters, either `joy ` or `key `. +0x00 */
    int mPadIndex; /*!< Which controller produced the reading. +0x04 */
    int mButton;   /*!< Raw button identifier, which the jump table indexes by less one. +0x08 */
    float mValue;  /*!< Reading value. A button is pressed while the value is above zero. +0x0c */
};

/**
 * Translator from one raw controller reading to one front-end command.
 *
 * The class emits no RTTI, so no descriptor, accessor, or vtable in the image bears its name. It
 * declares no allocation operator of its own either, because MetRenderer's constructor allocates
 * it through the tagged allocator with an explicit size rather than through a class operator, so
 * the deallocation-tag lever does not reach it. Its translation unit spans roughly `0x002e33f0`
 * through `0x002e72bc`, between MetMemDetectStartup and MetModeScreen, and no literal in that span
 * identifies a file or a function. The name here is therefore inferred from what the class does and
 * is not attested anywhere in the image.
 *
 * The object is twelve bytes and its one member is a `std::vector` of twelve-byte records, which
 * the destructor at `0x00371138` fixes by dividing the byte span by twelve. MetRenderer's
 * constructor sizes the vector at six. Each record is a `std::list` at `+0x00` over twenty-four
 * byte nodes, a count at `+0x04`, and a byte flag at `+0x08`, which the prototype the constructor
 * copies fixes. The element type of the list is not recovered, so the member is recorded here
 * rather than declared.
 *
 * The destructor is emitted into MetRenderer's translation unit rather than the class's own, which
 * is what an inline destructor compiles to.
 */
class MetCommandMap {
public:
    /**
     * Build the six mapping records.
     *
     * The body is not written. It zeroes the vector, sets the allocation tag `stl_maptree` for an
     * eight-byte element, builds one prototype record with an empty list, and resizes the vector to
     * six copies of it.
     *
     * @ghidraAddress 0x002e33f0
     */
    MetCommandMap();

    /**
     * Release every record and the vector.
     *
     * The body is not written. It unlinks each record's list and returns every node to the pool.
     *
     * @ghidraAddress 0x00371138
     */
    ~MetCommandMap();

    /**
     * Translate one controller reading into one front-end command.
     *
     * The body is not written. It copies the reading's mPadIndex into the command's mPadIndex,
     * responds to a `key ` reading with command 0x0f whenever mValue is above zero, and dispatches
     * a `joy ` reading through a 103-entry jump table at `0x007fd8a0` indexed by mButton less one.
     * Each arm of that table compares mValue against zero and yields one command code. A reading of
     * any other tag produces no command and the command's mCommand retains whatever it held.
     *
     * @param pReading The reading, which is the RawControllerMsg payload rather than the message.
     * @param pCommand The command the reading translates to.
     * @return Non-zero when a command was produced.
     * @ghidraAddress 0x002e3738
     */
    int Translate(const MetControllerReading *pReading, MetScreenCommand *pCommand);
};

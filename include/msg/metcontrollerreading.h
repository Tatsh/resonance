#pragma once

#include <iostream>

class IBStream;
class OBStream;

/**
 * One controller reading, as RawControllerMsg carries it.
 *
 * The four fields are recovered rather than inferred. RawControllerMsg::Clone() at `0x003da200`
 * copies them, RawController's one virtual receives them as three ints followed by a float in
 * this order, and MetRenderer::HandleMessage() passes the address of the first of them to
 * MetCommandMap::Translate(). The record sits at `+0x04` of the message, so the three words
 * MetRenderer reads at message offsets 4, 8, and 0x0c are mTag, mPadIndex, and mButton.
 *
 * The name is inferred. The record has no RTTI, and the program titles its two stream operators
 * `ControllerReading`. Its three routines sit together at `0x00100f40` through `0x00101120`,
 * directly after the Sequencer template's members, which places them in one translation unit.
 *
 * Every member is public, because MetRenderer, MetCommandMap, GrooveWorld, and ControllerCmd all
 * read the record directly and the image exposes no accessor.
 */
struct MetControllerReading {
    /**
     * Write the reading to a diagnostic stream as the device label, the pad index, the button,
     * and the value.
     *
     * The label is `key`, `joy`, `mouse`, or `none` for the four tags the routine recognises, and
     * nothing for any other tag. The three numbers follow separated by `.`, `.`, and `:`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00100f40
     */
    void Print(std::ostream &stream);

    int mTag;      /*!< Four characters, `joy `, `key `, `mous`, or `none`. +0x00 */
    int mPadIndex; /*!< Which controller produced the reading. +0x04 */
    int mButton;   /*!< Raw button identifier, which the jump table indexes by less one. +0x08 */
    float mValue;  /*!< Reading value. A button is pressed while the value is above zero. +0x0c */
};

/**
 * Write a controller reading as four four-byte transfers in member order.
 *
 * @param stream The stream to write to.
 * @param reading The reading.
 * @return The stream.
 * @ghidraAddress 0x00101060
 */
OBStream &operator<<(OBStream &stream, const MetControllerReading &reading);

/**
 * Read a controller reading back as four four-byte transfers in member order.
 *
 * The tag is read into a local first and stored after the other three fields.
 *
 * @param stream The stream to read from.
 * @param reading The reading to fill.
 * @return The stream.
 * @ghidraAddress 0x00101120
 */
IBStream &operator>>(IBStream &stream, MetControllerReading &reading);

#pragma once

class HxStream;

/**
 * Variable-length quantity, seven bits per byte, most significant group first.
 *
 * The encoding is the one Standard MIDI Files use. Mid::Reader reads delta times and meta lengths
 * through it, and the two string readers in HxStream's translation unit read their length prefixes
 * through it. The object is the one word below. A routine of the class takes the word's address in
 * a0.
 * Its name comes from the debugging symbols of the North American demo release.
 */
class HxVarLenNumber {
public:
    /**
     * Construct with the quantity unset, for Read() to fill.
     */
    HxVarLenNumber() {
    }

    /**
     * Store a quantity for Write().
     *
     * @param nValue The quantity.
     */
    explicit HxVarLenNumber(int nValue) : mValue(nValue) {
    }

    /**
     * Report the quantity.
     *
     * @return The quantity.
     */
    operator int() const {
        return mValue;
    }

    /**
     * Write the quantity.
     *
     * The groups are packed into one word, lowest group in the lowest byte and every higher group
     * marked with the continuation bit, and the word's bytes are written lowest first through
     * HxStream::WriteNum(). The value is shifted arithmetically. A negative value therefore never
     * terminates. The shipped program does not call it.
     *
     * @param stream The stream to write to.
     * @return The stream.
     * @ghidraAddress NTSC-U/C: 0x00405ad8
     * @ghidraAddress PAL: 0x0043f3c8
     */
    HxStream &Write(HxStream &stream) const;

    /**
     * Read a quantity.
     *
     * The value is cleared and each byte read through HxStream::ReadNum() adds its low seven bits
     * after a seven-bit shift, until a byte with its top bit clear ends the quantity.
     *
     * @param stream The stream to read from.
     * @return The stream.
     * @ghidraAddress NTSC-U/C: 0x00405b70
     * @ghidraAddress PAL: 0x0043f460
     */
    HxStream &Read(HxStream &stream);

    int mValue; /*!< The quantity. */
};

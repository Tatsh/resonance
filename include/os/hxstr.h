#pragma once

#include <iostream>

#include "os/mem.h"

/**
 * Heap-allocated NUL-terminated string.
 *
 * Titled after `HxStr.cpp`, the file recorded by its own asserts. The member `mStr` comes from the
 * text of the assert `mStr != 0`, and `mLen` from `pos <= mLen`. The class is not polymorphic and
 * has no RTTI, so it has no vptr. Its destructor is inlined at every call site in the image, which
 * is why it is defined here.
 *
 * An empty string is represented by a null `mStr` with `mLen` zero rather than by a pointer to an
 * empty buffer. Every accessor that dereferences `mStr` therefore asserts first, and the
 * comparison operators treat a null pointer and an empty buffer as equal.
 *
 * `mLen` excludes the terminator, and the buffer is always `mLen + 1` bytes.
 *
 * Both members are public, and each records its own evidence below. The comparison operators are
 * written as members because a member and a free function with the string as its first operand
 * compile to the same two-argument call, which leaves the image unable to distinguish them.
 */
class HxStr {
public:
    /**
     * Construct an empty string.
     *
     * Defined in the header rather than compiled out of line, which is why it has no address of
     * its own. A static initialisation that constructs a string global zeroes the two members in
     * place with no call, which is what establishes both that the constructor exists and that it
     * does nothing beyond producing the empty representation.
     */
    HxStr() : mLen(0), mStr(nullptr) {
    }

    /**
     * Construct a copy of a C string.
     *
     * A null argument produces an empty string with no allocation.
     *
     * @param pszText The text to copy.
     * @ghidraAddress 0x004b7ad0
     */
    HxStr(const char *pszText);

    /**
     * Construct a copy of another string.
     *
     * @param other The string to copy.
     * @ghidraAddress 0x004b7b50
     */
    HxStr(const HxStr &other);

    /**
     * Construct a run of one repeated character.
     *
     * @param nCount The number of characters, which becomes the length.
     * @param ch The character to repeat.
     * @ghidraAddress 0x004b7bd0
     */
    HxStr(unsigned nCount, char ch);

    ~HxStr() {
        if (mStr != nullptr) {
            MemFree(mStr);
        }
    }

    /**
     * Append another string.
     *
     * @param other The string to append.
     * @return This string.
     * @ghidraAddress 0x004b7c68
     */
    HxStr &operator+=(const HxStr &other);

    /**
     * Append a C string.
     *
     * The routine builds the temporary string itself rather than taking one a call site built,
     * which is what establishes that the overload exists rather than callers converting and
     * reaching the HxStr overload. The temporary is released before returning.
     *
     * The body is inline. Units that use it emit their own identical out-of-line copy, at
     * `0x00183a70`, `0x00254870`, `0x0028c198`, `0x0030d618`, `0x0038fc58`, `0x003fc688`,
     * `0x00429500`, `0x00431c30`, `0x00453d88`, `0x004beb98`, `0x004dfaa0`, and `0x0050cc00`.
     *
     * @param pszText The text to append.
     * @return This string.
     * @ghidraAddress 0x0010edb0
     * @ghidraAddress 0x00183a70
     */
    HxStr &operator+=(const char *pszText) {
        return *this += HxStr(pszText);
    }

    /**
     * Append one character.
     *
     * @param ch The character to append.
     * @return This string.
     * @ghidraAddress 0x004b7d28
     */
    HxStr &operator+=(char ch);

    /**
     * Replace this string with a copy of a C string.
     *
     * @param pszText The text to copy, or null for an empty string.
     * @return This string.
     * @ghidraAddress 0x004b7dd8
     */
    HxStr &operator=(const char *pszText);

    /**
     * Replace this string with a copy of another.
     *
     * @param other The string to copy.
     * @return This string.
     * @ghidraAddress 0x004b7e78
     */
    HxStr &operator=(const HxStr &other);

    /**
     * Read one character.
     *
     * The index is permitted to equal the length, which reads the terminator.
     *
     * @param i The index.
     * @return The character.
     * @ghidraAddress 0x004b7f18
     */
    char operator[](unsigned i) const;

    /**
     * Compare against a C string for inequality.
     *
     * A null pointer and an empty buffer compare equal.
     *
     * @param pszRight The text to compare against.
     * @return True when the two differ.
     * @ghidraAddress 0x004b7f98
     */
    bool operator!=(const char *pszRight) const;

    /**
     * Compare against another string for inequality.
     *
     * @param right The string to compare against.
     * @return True when the two differ.
     * @ghidraAddress 0x004b8018
     */
    bool operator!=(const HxStr &right) const;

    /**
     * Compare against a C string for equality.
     *
     * @param pszRight The text to compare against.
     * @return True when the two agree.
     * @ghidraAddress 0x004b80a0
     */
    bool operator==(const char *pszRight) const;

    /**
     * Compare against another string for equality.
     *
     * @param right The string to compare against.
     * @return True when the two agree.
     * @ghidraAddress 0x004b8120
     */
    bool operator==(const HxStr &right) const;

    /**
     * Order against another string.
     *
     * An empty string sorts before every non-empty string.
     *
     * @param right The string to compare against.
     * @return True when this string sorts before the other.
     * @ghidraAddress 0x004b81a8
     */
    bool operator<(const HxStr &right) const;

    /**
     * Discard the text and reserve a zero-filled buffer.
     *
     * The previous buffer is released through MemFreeScalar() rather than through MemFree(), which
     * is the one place in HxStr that uses the scalar release path. The new length is recorded
     * before the allocation is checked.
     *
     * @param nLen The new length, excluding the terminator.
     * @ghidraAddress 0x004b8230
     */
    void Alloc(unsigned nLen);

    /**
     * Find the first occurrence of a character.
     *
     * @param ch The character to look for.
     * @return The index, or -1 when the character is absent.
     * @ghidraAddress 0x004b82b8
     */
    int Find(char ch) const;

    /**
     * Find the first occurrence of a character at or after a position.
     *
     * @param ch The character to look for.
     * @param nStart The index to start from.
     * @return The index from the start of the string, or -1 when the character is absent.
     * @ghidraAddress 0x004b8350
     */
    int Find(char ch, unsigned nStart) const;

    /**
     * Find the first occurrence of a substring.
     *
     * @param pszText The text to look for.
     * @return The index, or -1 when the text is absent.
     * @ghidraAddress 0x004b83f0
     */
    int Find(const char *pszText) const;

    /**
     * Find the last occurrence of a character.
     *
     * @param ch The character to look for.
     * @return The index, or -1 when the character is absent.
     * @ghidraAddress 0x004b84b0
     */
    int ReverseFind(char ch) const;

    /**
     * Find the last occurrence of any of a set of characters.
     *
     * @param pszChars The candidate characters, as a NUL-terminated set.
     * @return The highest index at which any candidate appears, or -1 when none appears and when
     *         the argument is null.
     * @ghidraAddress 0x004b8550
     */
    int ReverseFindOneOf(const char *pszChars) const;

    /**
     * Compare a run of this string against a C string.
     *
     * @param pos The index to compare from.
     * @param len The number of characters to compare.
     * @param str The text to compare against.
     * @return A negative value, zero, or a positive value, as `strncmp()` defines it.
     * @ghidraAddress 0x004b8678
     */
    int Compare(unsigned pos, unsigned len, const char *str) const;

    /**
     * Extract the text from a position to the end.
     *
     * @param pos The index to start from.
     * @return The extracted text.
     * @ghidraAddress 0x004b8738
     */
    HxStr Mid(unsigned pos) const;

    /**
     * Extract a run of text.
     *
     * A run that would pass the end is shortened to the text from pos onward.
     *
     * @param pos The index to start from.
     * @param len The number of characters to extract.
     * @return The extracted text.
     * @ghidraAddress 0x004b78b8
     */
    HxStr Mid(unsigned pos, unsigned len) const;

    /**
     * Overwrite one character.
     *
     * Only a single-character replacement is implemented. A run of any other length trips the
     * `len == 1` assert and is then written as a single character regardless.
     *
     * @param pos The index to overwrite.
     * @param len The number of characters to overwrite, which must be 1.
     * @param ch The replacement character.
     * @return This string.
     * @ghidraAddress 0x004b8818
     */
    HxStr &Replace(unsigned pos, unsigned len, char ch);

    /**
     * Overwrite a run of text with another string.
     *
     * A run that would pass the end is shortened to the text from pos onward.
     *
     * @param pos The index to overwrite from.
     * @param len The number of characters to overwrite.
     * @param other The replacement text.
     * @return This string.
     * @ghidraAddress 0x004b88d0
     */
    HxStr &Replace(unsigned pos, unsigned len, const HxStr &other);

    /**
     * Empty the string in place.
     *
     * This is a distinct routine rather than Truncate(0). It tolerates an empty string, where
     * Truncate() would assert, and it asserts nothing itself. The buffer is retained.
     *
     * @return This string.
     * @ghidraAddress 0x004b89f0
     */
    HxStr &Clear();

    /**
     * Shorten the string in place.
     *
     * The buffer is not reallocated, so the excess capacity survives until the next assignment.
     *
     * @param pos The index to cut at, which must not exceed the current length.
     * @return This string.
     * @ghidraAddress 0x004b8a10
     */
    HxStr &Truncate(unsigned pos);

    /**
     * Remove a run of text.
     *
     * The tail is shifted down in place with no reallocation. A run that would pass the end
     * shortens the string to pos instead.
     *
     * @param pos The index to remove from.
     * @param len The number of characters to remove.
     * @return This string.
     * @ghidraAddress 0x004b8a98
     */
    HxStr &Erase(unsigned pos, unsigned len);

    /**
     * Insert a run of one repeated character.
     *
     * @param pos The index to insert at.
     * @param nCount The number of characters to insert.
     * @param ch The character to insert.
     * @return This string.
     * @ghidraAddress 0x004b8bd8
     */
    HxStr &Insert(unsigned pos, unsigned nCount, char ch);

    /**
     * Insert another string.
     *
     * @param pos The index to insert at.
     * @param other The text to insert.
     * @return This string.
     * @ghidraAddress 0x004b8cd8
     */
    HxStr &Insert(unsigned pos, const HxStr &other);

    /**
     * Write this string to a stream.
     *
     * The same write as operator<<(), with the string as the receiver, and likewise without a null
     * check. The image has no caller. The title is inferred.
     *
     * @param stream The stream to write to.
     * @return The stream.
     * @ghidraAddress 0x004b8e00
     */
    std::ostream &Print(std::ostream &stream) const;

    /**
     * Length excluding the terminator.
     *
     * Public because nine sites across four files in other subsystems read this member and `mStr`
     * directly, and the image exposes no accessor for either. A trivial inline accessor and a
     * public member emit the same single load, so the evidence cannot distinguish them, and the
     * public member is preferred because it adds no function that no address can be attached to.
     *
     * +0x00
     */
    unsigned mLen;

    /**
     * Text buffer, null while the string is empty.
     *
     * Public for the same reason as mLen. An empty string is a null buffer rather than a pointer
     * to a terminator, so every reader tests for null first.
     *
     * +0x04
     */
    char *mStr;

private:
    // Adopts a buffer of nLen + 1 bytes that the caller has already filled and terminated. The
    // name is inferred. The constructor is inlined at its one call site, in Mid().
    HxStr(char *pOwnedText, unsigned nLen) : mLen(nLen), mStr(pOwnedText) {
    }
};

/**
 * Concatenate two strings.
 *
 * Every call site inlines the body, so the operator has no address of its own. The body is
 * recovered from that emission rather than invented. A concatenation compiles to a copy
 * construction of the left operand into a temporary, the matching operator+=() on the temporary,
 * and a second copy construction of the result into the destination. `0x0017a79c` through
 * `0x0017a7b4` in SaveRemixMCT::ListRemixDir() and `0x00179de8` through `0x00179e08` are the two
 * clearest copies, and the first temporary is released immediately afterwards at each one.
 *
 * Whether the original wrote a free function or a const member cannot be settled, because a member
 * taking one operand and a free function taking two compile the same way once both are inlined.
 * The free form is used here for the same reason the comparison operators use the member form,
 * which is that one of the two has to be chosen.
 *
 * @param left The string the result starts with.
 * @param right The string appended to it.
 * @return The concatenation.
 */
inline HxStr operator+(const HxStr &left, const HxStr &right) {
    HxStr result(left);
    result += right;
    return result;
}

/**
 * Concatenate a string and a C string.
 *
 * Recovered on the same evidence as the overload above, from the triple at `0x0017a80c` through
 * `0x0017a824` and the one at `0x0038b1d4` through `0x0038b1ec` in
 * Rnd::MetScreen::ResolveContainerViews(). The middle call of each triple is the C string
 * operator+=() at `0x0010edb0` rather than the string one at `0x004b7c68`, which is what
 * distinguishes this overload from a call site converting its operand first.
 *
 * @param left The string the result starts with.
 * @param pszRight The text appended to it.
 * @return The concatenation.
 */
inline HxStr operator+(const HxStr &left, const char *pszRight) {
    HxStr result(left);
    result += pszRight;
    return result;
}

/**
 * Substitute an empty HxStr passes in place of a null buffer.
 *
 * A string whose mStr is null is the empty representation, and a caller handing that string to an
 * interface taking a plain pointer substitutes this global rather than passing null. Twenty
 * routines across unrelated subsystems read it through the same idiom, which is what places the
 * declaration beside HxStr rather than in any one of them.
 *
 * @ghidraAddress 0x006fbd10
 */
extern const char *g_szEmptyString;

/**
 * Sentinel a search returns when it finds nothing.
 *
 * The word holds all ones and sits one word past the empty string in the same literal pool, which
 * is what places it beside HxStr rather than in any one of its six readers. Those readers span
 * unrelated subsystems, including the glyph mesh builder and the PyCXX sequence binding, whose
 * released form returns the standard string's own no-position constant from the member that reads
 * it here.
 *
 * @ghidraAddress 0x008211bc
 */
extern const unsigned g_nHxStrNoPosition;

/**
 * Write a string to a stream.
 *
 * The buffer is passed to the stream without a null check. An empty string therefore arrives at
 * the stream as a null pointer.
 *
 * @param stream The stream to write to.
 * @param text The string to write.
 * @return The stream.
 * @ghidraAddress 0x004b8e28
 */
std::ostream &operator<<(std::ostream &stream, const HxStr &text);

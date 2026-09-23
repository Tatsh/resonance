#pragma once

/** Punctuation pairs in g_aShiftJisSymbols, one for each printable ASCII symbol. */
constexpr int kShiftJisSymbolCount = 33;

/** Runs in g_aShiftJisRanges: the digits, the capital letters, and the small letters. */
constexpr int kShiftJisRangeCount = 3;

/** One ASCII symbol and its full-width Shift-JIS code. The title is inferred. */
struct ShiftJisSymbol {
    unsigned short mShiftJis; /*!< The two-byte code, lead byte high. */
    char mAscii;              /*!< The ASCII character. */
};

/** One run of consecutive characters that maps linearly. The title is inferred. */
struct ShiftJisRange {
    unsigned short mShiftJisBase; /*!< The code of the first character of the run. */
    unsigned short mAsciiBase;    /*!< The first ASCII character of the run. */
};

/**
 * The ASCII punctuation from space to tilde in ASCII order, followed by one zero entry.
 *
 * @ghidraAddress 0x007251a8
 */
extern const ShiftJisSymbol g_aShiftJisSymbols[kShiftJisSymbolCount + 1];

/**
 * The digit, capital, and small-letter runs, based at `0x824f`, `0x8260`, and `0x8281`.
 *
 * @ghidraAddress 0x00725230
 */
extern const ShiftJisRange g_aShiftJisRanges[kShiftJisRangeCount];

/**
 * Convert one full-width Shift-JIS character back to ASCII.
 *
 * A lead byte of `0x82` maps three trail-byte runs back onto the digits and the letters, and the
 * digit run covers one trail byte more than the ten digits. A lead byte of `0x81` is looked up in
 * g_aShiftJisSymbols by its trail byte alone. The title is inferred.
 *
 * @param pShiftJis The two bytes of the character.
 * @return The ASCII character, or zero when the character has none.
 * @ghidraAddress 0x00556750
 */
char DecodeShiftJisCharacter(const char *pShiftJis);

/**
 * Convert one ASCII character to its full-width Shift-JIS code.
 *
 * A code outside space to tilde is reported through the log as `bad ASCII code 0x%x`. The title
 * is inferred.
 *
 * @param cAscii The character.
 * @return The two-byte code, lead byte high, or zero for a character outside the tables.
 * @ghidraAddress 0x00556818
 */
unsigned short EncodeShiftJisCharacter(unsigned char cAscii);

/**
 * Convert Shift-JIS text back to ASCII.
 *
 * The source length is halved and every pair converted. The first pair with no ASCII form
 * replaces the whole result with `.Kanji.`. The shipped program does not call it, and the title is
 * inferred.
 *
 * @param pszShiftJis The text to convert.
 * @param pszAscii The destination, of at least half the source length plus one byte.
 * @ghidraAddress 0x00556928
 */
void ShiftJisToAscii(const char *pszShiftJis, char *pszAscii);

/**
 * Convert ASCII text to the Shift-JIS bytes `icon.sys` stores a title as.
 *
 * Every character becomes two bytes, lead byte first, and two zero bytes end the result.
 * `SaveFileMCT::BuildIconSys()` is its only caller, and the title is inferred from that use and
 * from the `.Kanji.` fallback of the reverse conversion.
 *
 * @param pszAscii The text to convert.
 * @param pszDest The destination, of at least twice the text length plus two bytes.
 * @ghidraAddress 0x00556a20
 */
void AsciiToShiftJis(const char *pszAscii, char *pszDest);

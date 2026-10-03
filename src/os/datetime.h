#pragma once

class HxStr;

/**
 * Write the console's local date and time as "MM/DD/YY, HH:MM".
 *
 * The clock is read with sceCdReadClock() and converted to local time. Each field is written as
 * the two decimal digits of its BCD byte. The day precedes the year and follows the month, in the
 * United States order. text is replaced, not appended to. A failed read or a clock whose status
 * byte is non-zero writes nothing.
 *
 * @param text Receives the date and time.
 * @return Whether text was written.
 * @ghidraAddress NTSC-U/C: 0x0053a108
 * @ghidraAddress PAL: 0x00579a38
 */
bool FormatCurrentDateTime(HxStr &text);

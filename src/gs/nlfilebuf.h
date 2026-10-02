#pragma once

#include <fstream>

/**
 * File buffer that records where its current output line began.
 *
 * `9nlfilebuf` in the RTTI descriptor whose type function is at `0x001a9600`, with filebuf as its
 * one base. MultiMuse::Print() is the one recovered user. It casts a stream's buffer to this class
 * and subtracts mLineStart from the stream's tellp() to find the current column. The constructor,
 * the table, and the code that maintains mLineStart are not recovered, and the file the class
 * belongs to is not identified.
 */
class nlfilebuf : public std::filebuf {
public:
    /**
     * Stream position at which the current output line began.
     *
     * The name is inferred from the one reader. +0x58
     */
    int mLineStart;
};

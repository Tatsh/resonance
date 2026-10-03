#pragma once

#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Write a string as a four-byte length followed by its text, with no terminator.
 *
 * Every packet that carries a string open-codes this sequence, and the image holds no out-of-line
 * copy, which is the shape of an inline function. The call sites are GameChatPacket::saveGuts() at
 * `0x003e8458`, its second emission at `0x003e8720` in TestArbiterPacket's table, and
 * SPJoinDenyPacket::saveGuts() at `0x003e5d58`. The name and the placement are inferred. The length
 * goes through OBStream::WriteLE() and the text through OBStream::Write() on the same stream,
 * and only the second call's result is returned.
 *
 * @param stream The stream to write to.
 * @param text The string to write.
 * @return The stream Write() returned.
 */
inline OBStream &SaveHxStr(OBStream &stream, const HxStr &text) {
    int length = text.mLen;
    stream.WriteLE(&length, sizeof(length));
    return stream.Write(text.mStr != nullptr ? text.mStr : g_szEmptyString, length);
}

/**
 * Read a string written by SaveHxStr() back in place.
 *
 * The string is resized through HxStr::Alloc() to the length read, and the text is then read into
 * its buffer. An empty string has no buffer, so the zero-byte read targets the shared empty
 * literal instead, exactly as the open-coded sequences do. The call sites are the restoreGuts()
 * members paired with the saveGuts() members listed on SaveHxStr().
 *
 * @param stream The stream to read from.
 * @param text The string to fill.
 * @return The stream Read() returned.
 */
inline IBStream &LoadHxStr(IBStream &stream, HxStr &text) {
    int length;
    stream.ReadLE(&length, sizeof(length));
    text.Alloc(length);
    return stream.Read(text.mStr != nullptr ? text.mStr : const_cast<char *>(g_szEmptyString),
                       length);
}

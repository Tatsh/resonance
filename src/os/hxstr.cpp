#include "os/hxstr.h"

#include <iostream>
#include <string.h>

#include "os/assert.h"
#include "os/mem.h"

namespace {

#ifndef VIDEO_STANDARD_PAL
// Both operands of every comparison operator run through this test first, so a
// null buffer and an empty buffer compare alike.
inline bool IsBlank(const char *pszText) {
    return pszText == nullptr || *pszText == '\0';
}
#endif

} // namespace

HxStr::HxStr(const char *pszText) {
    if (pszText == nullptr) {
        mLen = 0;
#ifdef VIDEO_STANDARD_PAL
        mStr = const_cast<char *>(g_szEmptyString);
#else
        mStr = nullptr;
#endif
        return;
    }
    mLen = strlen(pszText);
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, pszText);
}

HxStr::HxStr(const HxStr &other) {
    if (other.mStr == nullptr) {
        mLen = 0;
#ifdef VIDEO_STANDARD_PAL
        mStr = const_cast<char *>(g_szEmptyString);
#else
        mStr = nullptr;
#endif
        return;
    }
    mLen = other.mLen;
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, other.mStr);
}

HxStr::HxStr(unsigned nCount, char ch) : mLen(nCount) {
    mStr = new char[nCount + 1];
    HX_ASSERT(mStr != 0)
    for (unsigned i = 0; i < nCount; ++i) {
        mStr[i] = ch;
    }
    mStr[nCount] = '\0';
}

HxStr &HxStr::operator+=(const HxStr &other) {
    mLen += other.mLen;
    char *pNew = new char[mLen + 1];
    HX_ASSERT(pNew != 0)
#ifdef VIDEO_STANDARD_PAL
    if (mStr == g_szEmptyString) {
#else
    if (mStr == nullptr) {
#endif
        *pNew = '\0';
    } else {
        strcpy(pNew, mStr);
        delete[] mStr;
    }
    if (other.mStr != nullptr) {
        strcat(pNew, other.mStr);
    }
    mStr = pNew;
    return *this;
}

HxStr &HxStr::operator+=(char ch) {
    char *pNew = new char[mLen + 2];
    ++mLen;
    HX_ASSERT(pNew != 0)
#ifdef VIDEO_STANDARD_PAL
    if (mStr != g_szEmptyString) {
#else
    if (mStr != nullptr) {
#endif
        strcpy(pNew, mStr);
        delete[] mStr;
    }
    mStr = pNew;
    mStr[mLen - 1] = ch;
    mStr[mLen] = '\0';
    return *this;
}

HxStr &HxStr::operator=(const char *pszText) {
    if (pszText == mStr) {
        return *this;
    }
#ifdef VIDEO_STANDARD_PAL
    if (mStr != g_szEmptyString && mStr != nullptr) {
#else
    if (mStr != nullptr) {
#endif
        delete[] mStr;
    }
    if (pszText == nullptr) {
        mLen = 0;
#ifdef VIDEO_STANDARD_PAL
        mStr = const_cast<char *>(g_szEmptyString);
#else
        mStr = nullptr;
#endif
        return *this;
    }
    mLen = strlen(pszText);
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, pszText);
    return *this;
}

HxStr &HxStr::operator=(const HxStr &other) {
    if (&other == this) {
        return *this;
    }
#ifdef VIDEO_STANDARD_PAL
    if (mStr != g_szEmptyString && mStr != nullptr) {
        delete[] mStr;
    }
    if (other.mStr == g_szEmptyString) {
        mLen = 0;
        mStr = other.mStr;
        return *this;
    }
#else
    if (mStr != nullptr) {
        delete[] mStr;
    }
    if (other.mStr == nullptr) {
        mLen = 0;
        mStr = nullptr;
        return *this;
    }
#endif
    // The length is measured again rather than copied from other.mLen.
    mLen = strlen(other.mStr);
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, other.mStr);
    return *this;
}

char HxStr::operator[](unsigned i) const {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    HX_ASSERT(i <= mLen)
    return mStr[i];
}

void HxStr::Alloc(unsigned nLen) {
#ifdef VIDEO_STANDARD_PAL
    if (mStr != g_szEmptyString && mStr != nullptr) {
        delete[] mStr;
    }
#else
    if (mStr != nullptr) {
        delete mStr; // Yes, the binary releases the array through the single-object delete.
    }
#endif
    mLen = nLen;
    mStr = new char[nLen + 1];
    memset(mStr, 0, mLen + 1);
    HX_ASSERT(mStr != 0) // Yes, the binary checks the allocation only after writing through it.
}

int HxStr::Find(char ch) const {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    const char *pFound = mStr;
    while (*pFound != '\0' && *pFound != ch) {
        ++pFound;
    }
    if (*pFound == '\0') {
        return -1;
    }
    return pFound - mStr;
}

int HxStr::Find(char ch, unsigned nStart) const {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    if (nStart >= mLen) {
        return -1;
    }
    const char *pFound = mStr + nStart;
    while (*pFound != '\0' && *pFound != ch) {
        ++pFound;
    }
    if (*pFound == '\0') {
        return -1;
    }
    return pFound - mStr;
}

int HxStr::Find(const char *pszText) const {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    unsigned nNeedle = strlen(pszText);
    for (unsigned i = 0; i + nNeedle <= mLen; ++i) {
        if (strncmp(mStr + i, pszText, nNeedle) == 0) {
            return i;
        }
    }
    return -1;
}

int HxStr::ReverseFind(char ch) const {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    const char *pFound = mStr + mLen - 1;
    while (pFound != mStr && *pFound != ch) {
        --pFound;
    }
    if (*pFound != ch) {
        return -1;
    }
    return pFound - mStr;
}

int HxStr::ReverseFindOneOf(const char *pszChars) const {
    if (pszChars == nullptr) {
        return -1;
    }
    int nBest = -1;
    for (const char *pCandidate = pszChars; *pCandidate != '\0'; ++pCandidate) {
        int nAt = ReverseFind(*pCandidate);
        if (nAt != -1 && nAt > nBest) {
            nBest = nAt;
        }
    }
    return nBest;
}

int HxStr::Compare(unsigned pos, unsigned len, const char *str) const {
#ifdef VIDEO_STANDARD_PAL
    if (str == nullptr) {
        return -1;
    }
#else
    HX_ASSERT(mStr != 0)
    HX_ASSERT(str != 0)
#endif
    HX_ASSERT(pos <= mLen)
    return strncmp(mStr + pos, str, len);
}

HxStr HxStr::Mid(unsigned pos) const {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    HX_ASSERT(pos <= mLen)
    return HxStr(mStr + pos);
}

HxStr HxStr::Mid(unsigned pos, unsigned len) const {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    HX_ASSERT(pos <= mLen)
    if (pos + len >= mLen) {
        return Mid(pos);
    }
    char *pBuf = new char[len + 1];
    strncpy(pBuf, mStr + pos, len);
    pBuf[len] = '\0';
    // The binary copy-constructs the result from this adopting temporary and then frees it.
    HxStr part(pBuf, len);
    return part;
}

HxStr &HxStr::Replace(unsigned pos, unsigned len, char ch) {
    HX_ASSERT(len == 1)
#ifdef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != g_szEmptyString)
#else
    HX_ASSERT(mStr != 0)
#endif
    HX_ASSERT(pos <= mLen)
    mStr[pos] = ch;
    return *this;
}

HxStr &HxStr::Replace(unsigned pos, unsigned len, const HxStr &other) {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    HX_ASSERT(pos <= mLen)
    if (pos + len > mLen) {
        len = mLen - pos;
    }
    unsigned nTotal = mLen + other.mLen - len;
    char *pNew = new char[nTotal + 1];
    strncpy(pNew, mStr, pos);
    strcpy(pNew + pos, other.mStr);
    strcpy(pNew + pos + other.mLen, mStr + pos + len);
    mLen = nTotal;
    // Yes, the European binary also frees here without testing for the shared empty text.
    if (mStr != nullptr) {
        delete[] mStr;
    }
    mStr = pNew;
    return *this;
}

HxStr &HxStr::Clear() {
#ifdef VIDEO_STANDARD_PAL
    if (mStr != g_szEmptyString) {
#else
    if (mStr != nullptr) {
#endif
        mLen = 0;
        mStr[0] = '\0';
    }
    return *this;
}

HxStr &HxStr::Truncate(unsigned pos) {
#ifdef VIDEO_STANDARD_PAL
    HX_ASSERT(pos <= mLen)
    if (mStr != g_szEmptyString) {
        mLen = pos;
        mStr[pos] = '\0';
    }
#else
    HX_ASSERT(mStr != 0)
    HX_ASSERT(pos <= mLen)
    mLen = pos;
    mStr[pos] = '\0';
#endif
    return *this;
}

HxStr &HxStr::Erase(unsigned pos, unsigned len) {
#ifndef VIDEO_STANDARD_PAL
    HX_ASSERT(mStr != 0)
#endif
    HX_ASSERT(pos <= mLen)
    if (pos + len >= mLen) {
        return Truncate(pos);
    }
    // The final pass of the loop copies the terminator, so no separate
    // termination is needed.
    for (unsigned i = pos; i + len <= mLen; ++i) {
        mStr[i] = mStr[i + len];
    }
    mLen -= len;
    return *this;
}

HxStr &HxStr::Insert(unsigned pos, unsigned nCount, char ch) {
#ifdef VIDEO_STANDARD_PAL
    if (mStr == g_szEmptyString) {
#else
    if (mStr == nullptr) {
#endif
        // An insertion into an empty string writes one character whatever nCount
        // requests.
        mStr = new char[2];
        mLen = 1;
        mStr[0] = ch;
        mStr[1] = '\0';
        return *this;
    }
    unsigned nTotal = mLen + nCount;
    char *pNew = new char[nTotal + 1];
    strncpy(pNew, mStr, pos);
    for (unsigned i = pos; i < pos + nCount; ++i) {
        pNew[i] = ch;
    }
    strcpy(pNew + pos + nCount, mStr + pos);
    mLen = nTotal;
    delete[] mStr;
    mStr = pNew;
    return *this;
}

HxStr &HxStr::Insert(unsigned pos, const HxStr &other) {
#ifdef VIDEO_STANDARD_PAL
    if (mStr == g_szEmptyString) {
#else
    if (mStr == nullptr) {
#endif
        return *this = other;
    }
    unsigned nTotal = mLen + other.mLen;
    char *pNew = new char[nTotal + 1];
    strncpy(pNew, mStr, pos);
    strcpy(pNew + pos, other.mStr);
    strcpy(pNew + pos + other.mLen, mStr + pos);
    mLen = nTotal;
    delete[] mStr;
    mStr = pNew;
    return *this;
}

#ifdef VIDEO_STANDARD_PAL
bool HxStr::operator!=(const char *pszRight) const {
    if (pszRight == nullptr) {
        return true;
    }
    return strcmp(pszRight, mStr) != 0;
}

bool HxStr::operator!=(const HxStr &right) const {
    return strcmp(right.mStr, mStr) != 0;
}

bool HxStr::operator==(const char *pszRight) const {
    if (pszRight == nullptr) {
        return false;
    }
    return strcmp(pszRight, mStr) == 0;
}

bool HxStr::operator==(const HxStr &right) const {
    return strcmp(right.mStr, mStr) == 0;
}

bool HxStr::operator<(const HxStr &right) const {
    return strcmp(mStr, right.mStr) < 0;
}
#else
bool HxStr::operator!=(const char *pszRight) const {
    if (IsBlank(mStr) && IsBlank(pszRight)) {
        return false;
    }
    if (IsBlank(mStr) || IsBlank(pszRight)) {
        return true;
    }
    return strcmp(pszRight, mStr) != 0;
}

bool HxStr::operator!=(const HxStr &right) const {
    if (IsBlank(mStr) && IsBlank(right.mStr)) {
        return false;
    }
    if (IsBlank(mStr) || IsBlank(right.mStr)) {
        return true;
    }
    return strcmp(right.mStr, mStr) != 0;
}

bool HxStr::operator==(const char *pszRight) const {
    if (IsBlank(mStr) && IsBlank(pszRight)) {
        return true;
    }
    if (IsBlank(mStr) || IsBlank(pszRight)) {
        return false;
    }
    return strcmp(pszRight, mStr) == 0;
}

bool HxStr::operator==(const HxStr &right) const {
    if (IsBlank(mStr) && IsBlank(right.mStr)) {
        return true;
    }
    if (IsBlank(mStr) || IsBlank(right.mStr)) {
        return false;
    }
    return strcmp(right.mStr, mStr) == 0;
}

bool HxStr::operator<(const HxStr &right) const {
    if (IsBlank(mStr)) {
        return !IsBlank(right.mStr);
    }
    if (IsBlank(right.mStr)) {
        return false;
    }
    return strcmp(mStr, right.mStr) < 0;
}
#endif

std::ostream &HxStr::Print(std::ostream &stream) const {
    return stream << mStr;
}

std::ostream &operator<<(std::ostream &stream, const HxStr &text) {
    stream << text.mStr;
    return stream;
}

const char *g_szEmptyString = "";

const unsigned g_nHxStrNoPosition = 0xffffffffu;

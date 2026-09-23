#include "os/hxstr.h"

#include <iostream>
#include <string.h>

#include "os/assert.h"
#include "os/mem.h"

namespace {

// Both operands of every comparison operator run through this test first, so a
// null buffer and an empty buffer compare alike.
inline bool IsBlank(const char *pszText) {
    return pszText == nullptr || *pszText == '\0';
}

} // namespace

// 0x004b7ad0
HxStr::HxStr(const char *pszText) {
    if (pszText == nullptr) {
        mLen = 0;
        mStr = nullptr;
        return;
    }
    mLen = strlen(pszText);
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, pszText);
}

// 0x004b7b50
HxStr::HxStr(const HxStr &other) {
    if (other.mStr == nullptr) {
        mLen = 0;
        mStr = nullptr;
        return;
    }
    mLen = other.mLen;
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, other.mStr);
}

// 0x004b7bd0
HxStr::HxStr(unsigned nCount, char ch) {
    mLen = nCount;
    mStr = new char[nCount + 1];
    HX_ASSERT(mStr != 0)
    for (unsigned i = 0; i < nCount; ++i) {
        mStr[i] = ch;
    }
    mStr[nCount] = '\0';
}

// 0x004b7c68
HxStr &HxStr::operator+=(const HxStr &other) {
    mLen += other.mLen;
    char *pNew = new char[mLen + 1];
    HX_ASSERT(pNew != 0)
    if (mStr == nullptr) {
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

// 0x004b7d28
HxStr &HxStr::operator+=(char ch) {
    char *pNew = new char[mLen + 2];
    ++mLen;
    HX_ASSERT(pNew != 0)
    if (mStr != nullptr) {
        strcpy(pNew, mStr);
        delete[] mStr;
    }
    mStr = pNew;
    mStr[mLen - 1] = ch;
    mStr[mLen] = '\0';
    return *this;
}

// 0x004b7dd8
HxStr &HxStr::operator=(const char *pszText) {
    if (pszText == mStr) {
        return *this;
    }
    if (mStr != nullptr) {
        delete[] mStr;
    }
    if (pszText == nullptr) {
        mLen = 0;
        mStr = nullptr;
        return *this;
    }
    mLen = strlen(pszText);
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, pszText);
    return *this;
}

// 0x004b7e78
HxStr &HxStr::operator=(const HxStr &other) {
    if (&other == this) {
        return *this;
    }
    if (mStr != nullptr) {
        delete[] mStr;
    }
    if (other.mStr == nullptr) {
        mLen = 0;
        mStr = nullptr;
        return *this;
    }
    // The length is measured again rather than copied from other.mLen.
    mLen = strlen(other.mStr);
    mStr = new char[mLen + 1];
    HX_ASSERT(mStr != 0)
    strcpy(mStr, other.mStr);
    return *this;
}

// 0x004b7f18
char HxStr::operator[](unsigned i) const {
    HX_ASSERT(mStr != 0)
    HX_ASSERT(i <= mLen)
    return mStr[i];
}

// 0x004b8230
void HxStr::Alloc(unsigned nLen) {
    if (mStr != nullptr) {
        delete mStr; // Yes, the binary releases the array through the single-object delete.
    }
    mLen = nLen;
    mStr = new char[nLen + 1];
    memset(mStr, 0, mLen + 1);
    HX_ASSERT(mStr != 0) // Yes, the binary checks the allocation only after writing through it.
}

// 0x004b82b8
int HxStr::Find(char ch) const {
    HX_ASSERT(mStr != 0)
    const char *pFound = mStr;
    while (*pFound != '\0' && *pFound != ch) {
        ++pFound;
    }
    if (*pFound == '\0') {
        return -1;
    }
    return pFound - mStr;
}

// 0x004b8350
int HxStr::Find(char ch, unsigned nStart) const {
    HX_ASSERT(mStr != 0)
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

// 0x004b83f0
int HxStr::Find(const char *pszText) const {
    HX_ASSERT(mStr != 0)
    unsigned nNeedle = strlen(pszText);
    for (unsigned i = 0; i + nNeedle <= mLen; ++i) {
        if (strncmp(mStr + i, pszText, nNeedle) == 0) {
            return i;
        }
    }
    return -1;
}

// 0x004b84b0
int HxStr::ReverseFind(char ch) const {
    HX_ASSERT(mStr != 0)
    const char *pFound = mStr + mLen - 1;
    while (pFound != mStr && *pFound != ch) {
        --pFound;
    }
    if (*pFound != ch) {
        return -1;
    }
    return pFound - mStr;
}

// 0x004b8550
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

// 0x004b8678
int HxStr::Compare(unsigned pos, unsigned len, const char *str) const {
    HX_ASSERT(mStr != 0)
    HX_ASSERT(str != 0)
    HX_ASSERT(pos <= mLen)
    return strncmp(mStr + pos, str, len);
}

// 0x004b8738
HxStr HxStr::Mid(unsigned pos) const {
    HX_ASSERT(mStr != 0)
    HX_ASSERT(pos <= mLen)
    return HxStr(mStr + pos);
}

// 0x004b78b8
HxStr HxStr::Mid(unsigned pos, unsigned len) const {
    HX_ASSERT(mStr != 0)
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

// 0x004b8818
HxStr &HxStr::Replace(unsigned pos, unsigned len, char ch) {
    HX_ASSERT(len == 1)
    HX_ASSERT(mStr != 0)
    HX_ASSERT(pos <= mLen)
    mStr[pos] = ch;
    return *this;
}

// 0x004b88d0
HxStr &HxStr::Replace(unsigned pos, unsigned len, const HxStr &other) {
    HX_ASSERT(mStr != 0)
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
    if (mStr != nullptr) {
        delete[] mStr;
    }
    mStr = pNew;
    return *this;
}

// 0x004b89f0
HxStr &HxStr::Clear() {
    if (mStr != nullptr) {
        mLen = 0;
        mStr[0] = '\0';
    }
    return *this;
}

// 0x004b8a10
HxStr &HxStr::Truncate(unsigned pos) {
    HX_ASSERT(mStr != 0)
    HX_ASSERT(pos <= mLen)
    mLen = pos;
    mStr[pos] = '\0';
    return *this;
}

// 0x004b8a98
HxStr &HxStr::Erase(unsigned pos, unsigned len) {
    HX_ASSERT(mStr != 0)
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

// 0x004b8bd8
HxStr &HxStr::Insert(unsigned pos, unsigned nCount, char ch) {
    if (mStr == nullptr) {
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

// 0x004b8cd8
HxStr &HxStr::Insert(unsigned pos, const HxStr &other) {
    if (mStr == nullptr) {
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

// 0x004b7f98
bool HxStr::operator!=(const char *pszRight) const {
    if (IsBlank(mStr) && IsBlank(pszRight)) {
        return false;
    }
    if (IsBlank(mStr) || IsBlank(pszRight)) {
        return true;
    }
    return strcmp(pszRight, mStr) != 0;
}

// 0x004b8018
bool HxStr::operator!=(const HxStr &right) const {
    if (IsBlank(mStr) && IsBlank(right.mStr)) {
        return false;
    }
    if (IsBlank(mStr) || IsBlank(right.mStr)) {
        return true;
    }
    return strcmp(right.mStr, mStr) != 0;
}

// 0x004b80a0
bool HxStr::operator==(const char *pszRight) const {
    if (IsBlank(mStr) && IsBlank(pszRight)) {
        return true;
    }
    if (IsBlank(mStr) || IsBlank(pszRight)) {
        return false;
    }
    return strcmp(pszRight, mStr) == 0;
}

// 0x004b8120
bool HxStr::operator==(const HxStr &right) const {
    if (IsBlank(mStr) && IsBlank(right.mStr)) {
        return true;
    }
    if (IsBlank(mStr) || IsBlank(right.mStr)) {
        return false;
    }
    return strcmp(right.mStr, mStr) == 0;
}

// 0x004b81a8
bool HxStr::operator<(const HxStr &right) const {
    if (IsBlank(mStr)) {
        return !IsBlank(right.mStr);
    }
    if (IsBlank(right.mStr)) {
        return false;
    }
    return strcmp(mStr, right.mStr) < 0;
}

// 0x004b8e00
std::ostream &HxStr::Print(std::ostream &stream) const {
    return stream << mStr;
}

// 0x004b8e28
std::ostream &operator<<(std::ostream &stream, const HxStr &text) {
    stream << text.mStr;
    return stream;
}

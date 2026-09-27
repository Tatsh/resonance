#include "gs/museplayer.h"

#include <cstddef>

#include "os/mem.h"

// The image emits no out-of-line copy for either member.
// Every allocation of a derived class inlines the call.
// The destructor at 0x001aa3e8 releases through the global delete.
// That release identifies the untagged heap for both members.

// No out-of-line copy exists in the image.
void *MusePlayer::operator new(size_t nSize) {
    return ::operator new(nSize);
}

// No out-of-line copy exists in the image.
void MusePlayer::operator delete(void *pBlock) {
    ::operator delete(pBlock);
}

#include "gs/multimuse.h"

#include "msg/messageio.h"

// 0x001a9738
void MultiMuse::SaveFields(OBStream &stream) {
    const int nCount = mEntries.end() - mEntries.begin();
    stream.Write(&nCount, sizeof(nCount));

    std::vector<TickObj<MuseMsg *> >::iterator it = mEntries.begin();
    for (; it != mEntries.end(); ++it) {
        Mid::MBT position = it->mPosition;
        position.Save(stream);
        WriteMessagePointerToStream(stream, it->mValue);
    }
}
